/*
 * Copyright 2016-2018 Kacper Kasper, <kacperkasper@gmail.com>
 * Copyright 2021 Haiku, Inc.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Kacper Kasper
 *		Jacob Secunda
 */


#include "SaveWindow.h"

#include <Application.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <StringView.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SaveWindow"


namespace {
	const int kSemTimeOut = 50000;
	const int32 kMaxItems = 4;
}


SaveWindow::SaveWindow(const BStringList& unsavedFiles)
		:
		BWindow(BRect(100, 100, 200, 200), B_TRANSLATE("Unsaved files"),
			B_MODAL_WINDOW, B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_RESIZABLE
			| B_AUTO_UPDATE_SIZE_LIMITS, 0),
		fUnsavedFiles(unsavedFiles),
		fAlertValue(0),
		fCheckboxes(unsavedFiles.CountStrings(), NULL)
{
	_InitInterface();
	CenterOnScreen();
}


SaveWindow::~SaveWindow()
{
	if (fAlertSem >= B_OK)
		delete_sem(fAlertSem);
}


void
SaveWindow::_InitInterface()
{
	fMessageString = new BStringView("message",
		B_TRANSLATE("There are unsaved changes.\nSelect the files to save."));

	fSaveAll = new BButton(B_TRANSLATE("Save all"),
		new BMessage((uint32)SAVE_ALL));

	fSaveSelected = new BButton(B_TRANSLATE("Save selected"),
		new BMessage((uint32)SAVE_SELECTED));

	fDontSave = new BButton(B_TRANSLATE("Don't save"),
		new BMessage((uint32)DONT_SAVE));

	fCancel = new BButton(B_TRANSLATE("Cancel"),
		new BMessage(B_QUIT_REQUESTED));
	fCancel->MakeDefault(true);

	BGroupView* filesView = new BGroupView(B_VERTICAL, 0);
	filesView->SetViewUIColor(B_CONTROL_BACKGROUND_COLOR);

	fScrollView = new BScrollView("files", filesView, 0, false, true);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.Add(fMessageString)
		.Add(fScrollView)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(fSaveAll)
			.Add(fSaveSelected)
			.Add(fDontSave)
			.AddGlue()
			.Add(fCancel)
		.End()
		.SetInsets(B_USE_SMALL_INSETS);

	font_height fh;
	be_plain_font->GetHeight(&fh);
	float textHeight = fh.ascent + fh.descent + fh.leading + 5;

	int32 unsavedFileCount = fUnsavedFiles.CountStrings();

	fScrollView->SetExplicitSize(BSize(B_SIZE_UNSET,
		textHeight * std::min<uint32>(unsavedFileCount, kMaxItems)
		+ 25.0f));

	BScrollBar* bar = fScrollView->ScrollBar(B_VERTICAL);
	bar->SetSteps(textHeight / 2.0f, textHeight * 3.0f / 2.0f);
	bar->SetRange(0.0f, fUnsavedFiles.CountStrings() > kMaxItems ?
		(textHeight + 3.0f) * (unsavedFileCount - kMaxItems)
		: 0.0f);

	BGroupLayout* files = filesView->GroupLayout();
	files->SetInsets(B_USE_SMALL_INSETS);

	for (int32 index = 0; index < unsavedFileCount; ++index) {
		fCheckboxes[index] = new BCheckBox("file",
			fUnsavedFiles.StringAt(index), new BMessage((uint32)index));
		fCheckboxes[index]->SetValue(B_CONTROL_ON);
		files->AddView(fCheckboxes[index]);
	}
}


void
SaveWindow::Show()
{
	BWindow::Show();
	fScrollView->SetExplicitSize(BSize(Bounds().Width(), B_SIZE_UNSET));
}


// Method borrowed from BAlert.
std::vector<bool>
SaveWindow::Go()
{
	fAlertSem = create_sem(0, "AlertSem");
	if (fAlertSem < 0) {
		Quit();
		return std::vector<bool>(0);
	}

	// Get the originating window if it exists.
	BWindow* window = dynamic_cast<BWindow*>(
			BLooper::LooperForThread(find_thread(NULL)));

	Show();

	if (window != NULL) {
		status_t status = B_ERROR;
		while (status != B_BAD_SEM_ID) {
			do {
				status = acquire_sem_etc(fAlertSem, 1, B_RELATIVE_TIMEOUT,
					kSemTimeOut);
				// We've (probably) had our time slice taken away from us.
			} while (status == B_INTERRUPTED);

			window->UpdateIfNeeded();
		}
	} else {
		// There's no window to update, so just hang out until we're done.
		while (acquire_sem(fAlertSem) == B_INTERRUPTED);
	}

	// Have to cache the value since we delete on Quit().
	std::vector<bool> alertValues = fAlertValue;
	if (Lock())
		Quit();

	return alertValues;
}


void
SaveWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case SAVE_ALL:
			fAlertValue = std::vector<bool>(fUnsavedFiles.CountStrings(), true);
			break;

		case SAVE_SELECTED:
		{
			fAlertValue = std::vector<bool>(fUnsavedFiles.CountStrings(),
				false);
			for (uint32 index = 0; index < fCheckboxes.size(); ++index)
				fAlertValue[index] = fCheckboxes[index]->Value() ? true : false;
			break;
		}

		case DONT_SAVE:
		{
			fAlertValue = std::vector<bool>(fUnsavedFiles.CountStrings(),
				false);
			break;
		}

		default:
			BWindow::MessageReceived(message);
			return;
	}

	delete_sem(fAlertSem);
	fAlertSem = -1;
}
