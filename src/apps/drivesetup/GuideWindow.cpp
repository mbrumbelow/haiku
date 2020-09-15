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
	MSG_SIZE_SLIDER		= 'size',
};

GuideWindow::GuideWindow()
	:
	BWindow(BRect(50, 50, 600, 500), B_TRANSLATE_SYSTEM_NAME("DriveSetupGuided"),
		B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS),
	fCurrentDisk(NULL),
	fDiskSelectionMenu(NULL),
	fDiskSystemMenu(NULL),
	fBootPlatformMenu(NULL),
	fApplyButton(NULL),
	fCancelButton(NULL)
{

	// TODO: Custom view for pretty icon + disk selection
	BPopUpMenu* diskSelectionMenuPop = new BPopUpMenu(B_TRANSLATE("(none found)"));
	fDiskSelectionMenu = new BMenuField("disk selection",
                B_TRANSLATE("Drive:"), diskSelectionMenuPop);
	fDiskSelectionMenu->SetEnabled(false);

	fApplyButton = new BButton(B_TRANSLATE("Apply"), new BMessage(MSG_APPLY));
	fCancelButton = new BButton(B_TRANSLATE("Cancel"),
		new BMessage(MSG_CANCEL));

	// Disk System
	BPopUpMenu* diskSystemMenuPop = new BPopUpMenu(B_TRANSLATE("(none found)"));
	fDiskSystemMenu = new BMenuField("disk system selection",
                B_TRANSLATE("Disk System:"), diskSystemMenuPop);
	diskSystemMenuPop->AddItem(new BMenuItem(B_TRANSLATE("UEFI (Modern)"), NULL));
	diskSystemMenuPop->AddItem(new BMenuItem(B_TRANSLATE("BIOS (Legacy)"), NULL));
	fDiskSystemMenu->SetEnabled(true);

	// Boot Platform
	BPopUpMenu* bootPlatformMenuPop = new BPopUpMenu(B_TRANSLATE("(none found)"));
	fBootPlatformMenu = new BMenuField("boot platform selection",
                B_TRANSLATE("Bootloader:"), bootPlatformMenuPop);
	bootPlatformMenuPop->AddItem(new BMenuItem(B_TRANSLATE("GPT (Modern)"), NULL));
	bootPlatformMenuPop->AddItem(new BMenuItem(B_TRANSLATE("MBR (Legacy)"), NULL));
	fBootPlatformMenu->SetEnabled(true);

	// Reuse the "new partition" slider from Support.
	fSizeSlider = new SizeSlider("Slider", B_TRANSLATE("Operating system partition size"), NULL,
		0, 1024, kMegaByte);
        fSizeSlider->SetPosition(1.0);
        fSizeSlider->SetModificationMessage(new BMessage(MSG_SIZE_SLIDER));

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_WINDOW_SPACING)
		.Add(fDiskSelectionMenu)
		.Add(new BSeparatorView(B_HORIZONTAL))
		.Add(fDiskSystemMenu)
		.Add(fBootPlatformMenu)
		.Add(fSizeSlider)
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
GuideWindow::_SelectDisk()
{
	// fSizeSlider set to disk size
	// scan disk for existing partition systems + partitions
	// scan disk for free partition space
	// other sanity checks?
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

	if (diskSelectionMenuPop->CountItems() > 0) {
		diskSelectionMenuPop->SetLabelFromMarked(true);
		fDiskSelectionMenu->SetEnabled(true);
	}
}
