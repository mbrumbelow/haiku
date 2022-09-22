/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2021, Andrew Lindesay <apl@lindesay.co.nz>.
 * Copyright 2022, Niels Sascha Reedijk <niels.reedijk@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef SCREENSHOT_WINDOW_H
#define SCREENSHOT_WINDOW_H

#include <optional>

#include <DataIO.h>
#include <ExclusiveBorrow.h>
#include <HttpResult.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <ToolBar.h>
#include <Window.h>

#include "PackageInfo.h"
#include "WebAppInterface.h"


class BarberPole;
class BitmapView;
class BStringView;

using BPrivate::Network::BExclusiveBorrow;
using BPrivate::Network::BHttpResult;


enum {
	MSG_NEXT_SCREENSHOT     = 'nscr',
	MSG_PREVIOUS_SCREENSHOT = 'pscr',
	MSG_DOWNLOAD_START      = 'dlst',
};


class ScreenshotWindow : public BWindow {
public:
								ScreenshotWindow(BWindow* parent, BRect frame);
	virtual						~ScreenshotWindow();

	virtual bool				QuitRequested();

	virtual	void				MessageReceived(BMessage* message);

			void				SetOnCloseMessage(
									const BMessenger& messenger,
									const BMessage& message);

			void				SetPackage(const PackageInfoRef& package);

	static	void				CleanupIcons();

private:
			void				_DownloadScreenshot();

			BSize				_MaxWidthAndHeightOfAllScreenshots();
			void				_ResizeToFitAndCenter();
			void				_UpdateToolBar();

private:
	enum {
		kProgressIndicatorDelay = 200000 // us
	};

private:
			BMessenger			fOnCloseTarget;
			BMessage			fOnCloseMessage;

			BToolBar*			fToolBar;
			BarberPole*			fBarberPole;
			bool				fBarberPoleShown;
			BStringView*		fIndexView;

			BitmapRef			fScreenshot;
			BitmapView*			fScreenshotView;

			int32				fCurrentScreenshotIndex;

			PackageInfoRef		fPackage;

			WebAppInterface		fWebAppInterface;
			BMessageRunner		fDelayedProgressMessage;
			std::optional<BHttpResult>
								fDownload;
			std::optional<BExclusiveBorrow<BMallocIO>>
								fDownloadStream;
};


#endif // SCREENSHOT_WINDOW_H
