/*
 * Copyright 2002-2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */


#include "GuideWindow.h"

#include <Application.h>
#include <Catalog.h>
#include <DiskDevice.h>
#include <GridLayoutBuilder.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <MenuItem.h>
#include <Message.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <SeparatorView.h>

#include <cstdio>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GuideWindow"


enum {
	MSG_APPLY		= 'aply',
	MSG_CANCEL		= 'cncl',
	MSG_DISK_SELCHG		= 'schg',
};

GuideWindow::GuideWindow()
	:
	BWindow(BRect(50, 50, 600, 500), B_TRANSLATE_SYSTEM_NAME("DriveSetupGuided"),
		B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS),
	fCurrentDisk(NULL),
	fApplyButton(NULL),
	fCancelButton(NULL)
{

	// TODO: Custom view for pretty icon + disk selection
	BPopUpMenu* diskSelectionMenuPop = new BPopUpMenu(B_TRANSLATE("(none found)"));
	//diskSelectionMenuPop->AddItem(new BMenuItem(B_TRANSLATE("None found"), NULL));
	fDiskSelectionMenu = new BMenuField("disk selection",
                B_TRANSLATE("Drive:"), diskSelectionMenuPop);
	fDiskSelectionMenu->SetEnabled(false);

	fApplyButton = new BButton(B_TRANSLATE("Apply"), new BMessage(MSG_APPLY));
	fCancelButton = new BButton(B_TRANSLATE("Cancel"),
		new BMessage(MSG_CANCEL));

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(0, B_USE_DEFAULT_SPACING, 0, B_USE_WINDOW_SPACING)
		.Add(fDiskSelectionMenu)
		.AddGlue()
		.Add(new BSeparatorView(B_HORIZONTAL))
		.AddGroup(B_HORIZONTAL)
		.SetInsets(B_USE_WINDOW_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_WINDOW_SPACING, 0)
			.AddGlue()
			.Add(fCancelButton)
			.Add(fApplyButton);

	status_t status = fDiskDeviceRoster.StartWatching(BMessenger(this));
	if (status != B_OK) {
		fprintf(stderr, "Failed to start watching for device changes: %s\n",
			strerror(status));
	}

	CenterOnScreen();

	_ScanDisks();
}


GuideWindow::~GuideWindow()
{
	delete fCurrentDisk;
}


void
GuideWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
GuideWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	Hide();
	return false;
}

void
GuideWindow::_ScanDisks()
{
	BDiskDevice diskDevice;
	BMenu* diskSelectionMenuPop = fDiskSelectionMenu->Menu();

	fDiskDeviceRoster.RewindDevices();
	while (fDiskDeviceRoster.GetNextDevice(&diskDevice) == B_OK) {
		if (!diskDevice.HasMedia()
			|| diskDevice.IsReadOnlyMedia()
			|| diskDevice.IsWriteOnceMedia())
			continue;

		BPath devicePath;
		diskDevice.GetPath(&devicePath);
		diskSelectionMenuPop->AddItem(new BMenuItem(devicePath.Path(), NULL));
	}

	if (diskSelectionMenuPop->CountItems() > 0)
		fDiskSelectionMenu->SetEnabled(true);
}
