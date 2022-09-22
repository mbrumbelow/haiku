/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2017, Julian Harnath <julian.harnath@rwth-aachen.de>.
 * Copyright 2020-2021, Andrew Lindesay <apl@lindesay.co.nz>.
 * Copyright 2022, Niels Sascha Reedijk <niels.reedijk@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ScreenshotWindow.h"

#include <algorithm>

#include <Autolock.h>
#include <Catalog.h>
#include <ExclusiveBorrow.h>
#include <LayoutBuilder.h>
#include <MessageRunner.h>
#include <NetServicesDefs.h>
#include <StringView.h>

#include "BarberPole.h"
#include "BitmapView.h"
#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "WebAppInterface.h"

using namespace BPrivate::Network;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ScreenshotWindow"


static const rgb_color kBackgroundColor = { 51, 102, 152, 255 };
	// Drawn as a border around the screenshots and also what's behind their
	// transparent regions

static BitmapRef sNextButtonIcon(
	new(std::nothrow) SharedBitmap(RSRC_ARROW_LEFT), true);
static BitmapRef sPreviousButtonIcon(
	new(std::nothrow) SharedBitmap(RSRC_ARROW_RIGHT), true);


ScreenshotWindow::ScreenshotWindow(BWindow* parent, BRect frame)
	:
	BWindow(frame, B_TRANSLATE("Screenshot"),
		B_FLOATING_WINDOW_LOOK, B_FLOATING_SUBSET_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fBarberPoleShown(false),
	fDelayedProgressMessage(this, BMessage(MSG_DOWNLOAD_START), kProgressIndicatorDelay, 0)
{
	AddToSubset(parent);

	atomic_set(&fCurrentScreenshotIndex, 0);

	fBarberPole = new BarberPole("barber pole");
	fBarberPole->SetExplicitMaxSize(BSize(100, B_SIZE_UNLIMITED));
	fBarberPole->Hide();

	fIndexView = new BStringView("screenshot index", NULL);

	fToolBar = new BToolBar();
	fToolBar->AddAction(MSG_PREVIOUS_SCREENSHOT, this,
		sNextButtonIcon->Bitmap(BITMAP_SIZE_22),
		NULL, NULL);
	fToolBar->AddAction(MSG_NEXT_SCREENSHOT, this,
		sPreviousButtonIcon->Bitmap(BITMAP_SIZE_22),
		NULL, NULL);
	fToolBar->AddView(fIndexView);
	fToolBar->AddGlue();
	fToolBar->AddView(fBarberPole);

	fScreenshotView = new BitmapView("screenshot view");
	fScreenshotView->SetExplicitMaxSize(
		BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
	fScreenshotView->SetScaleBitmap(false);

	BGroupView* groupView = new BGroupView(B_VERTICAL);
	groupView->SetViewColor(kBackgroundColor);

	// Build layout
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(0, 3, 0, 0)
		.Add(fToolBar)
		.AddStrut(3)
		.AddGroup(groupView)
			.Add(fScreenshotView)
			.SetInsets(B_USE_WINDOW_INSETS)
		.End()
	;

	fScreenshotView->SetLowColor(kBackgroundColor);
		// Set after attaching all views to prevent it from being overriden
		// again by BitmapView::AllAttached()

	CenterOnScreen();
}


ScreenshotWindow::~ScreenshotWindow()
{

}


bool
ScreenshotWindow::QuitRequested()
{
	if (fOnCloseTarget.IsValid() && fOnCloseMessage.what != 0)
		fOnCloseTarget.SendMessage(&fOnCloseMessage);

	Hide();
	return false;
}


void
ScreenshotWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_NEXT_SCREENSHOT:
		{
			atomic_add(&fCurrentScreenshotIndex, 1);
			_UpdateToolBar();
			_DownloadScreenshot();
			break;
		}

		case MSG_PREVIOUS_SCREENSHOT:
			atomic_add(&fCurrentScreenshotIndex, -1);
			_UpdateToolBar();
			_DownloadScreenshot();
			break;

		case MSG_DOWNLOAD_START:
			if (!fBarberPoleShown && fDownload.has_value()) {
				fBarberPole->Start();
				fBarberPole->Show();
				fBarberPoleShown = true;
			}
			break;

		case UrlEvent::RequestCompleted:
		{
			HDINFO("Request completed");
			auto identifier = message->GetInt32(UrlEventData::Id, -1);
			auto success = message->GetBool(UrlEventData::Success, false);
			if (fDownload && fDownload->Identity() == identifier) {
				if (success) {
					HDINFO("got screenshot. Size: %" B_PRIuSIZE, (*fDownloadStream)->BufferLength());
					auto& download = *(fDownloadStream.value());
					fScreenshot = BitmapRef(new(std::nothrow)SharedBitmap(download), true);
					fScreenshotView->SetBitmap(fScreenshot);
					_ResizeToFitAndCenter();
				} else {
					// TODO: error message on error
					HDERROR("failed to download screenshot");
				}

				// update ui
				if (fBarberPoleShown) {
					fBarberPole->Hide();
					fBarberPole->Stop();
					fBarberPoleShown = true;
				}

				fDownload = std::nullopt;
				fDownloadStream = std::nullopt;
				fDelayedProgressMessage.SetCount(0);
			}
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
ScreenshotWindow::SetOnCloseMessage(
	const BMessenger& messenger, const BMessage& message)
{
	fOnCloseTarget = messenger;
	fOnCloseMessage = message;
}


void
ScreenshotWindow::SetPackage(const PackageInfoRef& package)
{
	if (fPackage == package)
		return;

	fPackage = package;

	BString title = B_TRANSLATE("Screenshot");
	if (package.IsSet()) {
		title = package->Title();
		_DownloadScreenshot();
	}
	SetTitle(title);

	atomic_set(&fCurrentScreenshotIndex, 0);

	_UpdateToolBar();
}


/* static */ void
ScreenshotWindow::CleanupIcons()
{
	sNextButtonIcon.Unset();
	sPreviousButtonIcon.Unset();
}


// #pragma mark - private

void
ScreenshotWindow::_DownloadScreenshot()
{
	if (!IsLocked()) {
		HDERROR("DownloadScreenshot: screenshot window must be locked");
		return;
	}

	ScreenshotInfoRef info;

	// Clear prevous
	fScreenshotView->UnsetBitmap();
	_ResizeToFitAndCenter();
	fDownload = std::nullopt;
	fDownloadStream = std::nullopt;

	// Get new screenshot data
	if (!fPackage.IsSet())
		HDINFO("package not set");
	else {
		if (fPackage->CountScreenshotInfos() == 0)
    		HDINFO("package has no screenshots");
    	else {
    		info = fPackage->ScreenshotInfoAtIndex(fCurrentScreenshotIndex);
    	}
	}

	if (!info.IsSet()) {
		HDINFO("screenshot not set");
		return;
	}

	// Retrieve screenshot from web-app
	fDownloadStream = make_exclusive_borrow<BMallocIO>();
	fDownload = fWebAppInterface.RetrieveScreenshotAsync(info->Code(),
		info->Width(), info->Height(), BBorrow<BDataIO>(*fDownloadStream), BMessenger(this));

	// Set up busy indicator to run only after the download takes a while
	fDelayedProgressMessage.SetCount(1);
}


BSize
ScreenshotWindow::_MaxWidthAndHeightOfAllScreenshots()
{
	BSize size(0, 0);

	// Find out dimensions of the largest screenshot of this package
	if (fPackage.IsSet()) {
		int count = fPackage->CountScreenshotInfos();
		for(int32 i = 0; i < count; i++) {
			const ScreenshotInfoRef& info = fPackage->ScreenshotInfoAtIndex(i);
			if (info.Get() != NULL) {
				float w = (float) info->Width();
				float h = (float) info->Height();
				if (w > size.Width())
					size.SetWidth(w);
				if (h > size.Height())
					size.SetHeight(h);
			}
		}
	}

	return size;
}


void
ScreenshotWindow::_ResizeToFitAndCenter()
{
	fScreenshotView->SetExplicitMinSize(_MaxWidthAndHeightOfAllScreenshots());
	Layout(false);

	// TODO: Limit window size to screen size (with a little margin),
	//       the image should then become scrollable.

	float minWidth;
	float minHeight;
	GetSizeLimits(&minWidth, NULL, &minHeight, NULL);
	ResizeTo(minWidth, minHeight);
	CenterOnScreen();
}


void
ScreenshotWindow::_UpdateToolBar()
{
	const int32 numScreenshots = fPackage->CountScreenshotInfos();
	const int32 currentIndex = atomic_get(&fCurrentScreenshotIndex);

	fToolBar->SetActionEnabled(MSG_PREVIOUS_SCREENSHOT,
		currentIndex > 0);
	fToolBar->SetActionEnabled(MSG_NEXT_SCREENSHOT,
		currentIndex < numScreenshots - 1);

	BString text;
	text << currentIndex + 1;
	text << " / ";
	text << numScreenshots;
	fIndexView->SetText(text);
}
