/*
 * Copyright 2020, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */


#include "LiveUpdatingMenu.h"

#include <string.h>

#include <Application.h>
#include <Catalog.h>
#include <Locale.h>
#include <MenuWindow.h>
#include <MenuItem.h>
#include <Window.h>

#include "Commands.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ContainerWindow"


//	#pragma mark - TLiveUpdatingMenu


TLiveUpdatingMenu::TLiveUpdatingMenu(const char* label)
	:
	BMenu(label)
{
}


TLiveUpdatingMenu::~TLiveUpdatingMenu()
{
}


void
TLiveUpdatingMenu::MessageReceived(BMessage* message)
{
	if (message != NULL && message->what == B_MODIFIERS_CHANGED) {
		UpdateMenuItemsForModifiersChanged();
	} else
		BMenu::MessageReceived(message);
}


void
TLiveUpdatingMenu::UpdateMenuItemsForModifiersChanged()
{
	// hook method
}


//	#pragma mark - TLiveUpdatingPopUpMenu


TLiveUpdatingPopUpMenu::TLiveUpdatingPopUpMenu(const char* label,
	bool radioMode = true, bool labelFromMarked, menu_layout layout)
	:
	BPopUpMenu(label, radioMode, labelFromMarked, layout)
{
}


TLiveUpdatingPopUpMenu::~TLiveUpdatingPopUpMenu()
{
}


void
TLiveUpdatingPopUpMenu::MessageReceived(BMessage* message)
{
	if (message != NULL && message->what == B_MODIFIERS_CHANGED)
		UpdateMenuItemsForModifiersChanged();
	else
		BMenu::MessageReceived(message);
}


void
TLiveUpdatingPopUpMenu::UpdateMenuItemsForModifiersChanged()
{
	// hook method
}


//	#pragma mark - TLiveUpdatingArrangeByMenu


TLiveUpdatingArrangeByMenu::TLiveUpdatingArrangeByMenu(const char* label)
	:
	TLiveUpdatingMenu(label)
{
}


TLiveUpdatingArrangeByMenu::~TLiveUpdatingArrangeByMenu()
{
}


void
TLiveUpdatingArrangeByMenu::LayoutChanged()
{
	BMenuWindow* menuWindow = dynamic_cast<BMenuWindow*>(Window());
	if (menuWindow != NULL)
		menuWindow->ResizeTo(Frame().Width(), menuWindow->Bounds().Height());

	BMenu::LayoutChanged();
}


void
TLiveUpdatingArrangeByMenu::UpdateMenuItemsForModifiersChanged()
{
	if (Window()->LockLooper()) {
		// Clean up/Clean up all
		BMenuItem* cleanUp = FindItem(kCleanup);
		if (cleanUp == NULL)
			cleanUp = FindItem(kCleanupAll);
		UpdateCleanUpMenuItem(cleanUp);

		Window()->UnlockLooper();
	}
}


void
TLiveUpdatingArrangeByMenu::UpdateCleanUpMenuItem(BMenuItem* item)
{
	if (item == NULL || item->Menu() == NULL || item->Message() == NULL)
		return;

	if ((modifiers() & B_SHIFT_KEY) != 0) {
		item->SetLabel(B_TRANSLATE("Clean up all"));
		item->SetShortcut('K', B_COMMAND_KEY | B_SHIFT_KEY);
		item->Message()->what = kCleanupAll;
	} else {
		item->SetLabel(B_TRANSLATE("Clean up"));
		item->SetShortcut('K', B_COMMAND_KEY);
		item->Message()->what = kCleanup;
	}
}


//	#pragma mark - TFileMixin


void
TFileMixin::UpdateCreateLinkMenuItem(BMenuItem* item)
{
	if (item == NULL || item->Menu() == NULL || item->Message() == NULL)
		return;

	if ((modifiers() & B_SHIFT_KEY) != 0) {
		item->SetLabel(B_TRANSLATE("Create relative link"));
		item->Message()->what = kCreateRelativeLink;
	} else {
		item->SetLabel(B_TRANSLATE("Create link"));
		item->Message()->what = kCreateLink;
	}
}


void
TFileMixin::UpdateCreateLinkMenuItemsInMenu(BMenu* menu)
{
	for (int32 i = menu->CountItems(); i-- > 0;) {
		BMenuItem* item = menu->ItemAt(i);
		if (item == NULL || item->Message() == NULL)
			continue; // invalid

		uint32 what = item->Message()->what;
		if (what != kCreateLink && what != kCreateRelativeLink)
			continue; // not our menu item

		const char* label = BString(item->Label()).Trim().String();
			// make sure to trim off the extra spaces I added
		if (strcmp(label, B_TRANSLATE("Create link")) == 0
			|| strcmp(label, B_TRANSLATE("Create relative link")) == 0) {
			// only update label if "Create link" or "Create relative link"
			UpdateCreateLinkMenuItem(item);
		} else {
			// only update the item message what
			item->Message()->what = ((modifiers() & B_SHIFT_KEY) != 0
				? kCreateRelativeLink : kCreateLink);
		}
	}
}


void
TFileMixin::UpdateCutMenuItem(BMenuItem* item)
{
	if (item == NULL || item->Menu() == NULL || item->Message() == NULL)
		return;

	if ((modifiers() & B_SHIFT_KEY) != 0) {
		item->SetLabel(B_TRANSLATE("Cut more"));
		item->SetShortcut('X', B_COMMAND_KEY | B_SHIFT_KEY);
		item->SetMessage(new BMessage(kCutMoreSelectionToClipboard));
	} else {
		item->SetLabel(B_TRANSLATE("Cut"));
		item->SetShortcut('X', B_COMMAND_KEY);
		item->SetMessage(new BMessage(B_CUT));
	}
}


void
TFileMixin::UpdateCopyMenuItem(BMenuItem* item)
{
	if (item == NULL || item->Menu() == NULL || item->Message() == NULL)
		return;

	if ((modifiers() & B_SHIFT_KEY) != 0) {
		item->SetLabel(B_TRANSLATE("Copy more"));
		item->SetShortcut('C', B_COMMAND_KEY | B_SHIFT_KEY);
		item->Message()->what = kCopyMoreSelectionToClipboard;
	} else {
		item->SetLabel(B_TRANSLATE("Copy"));
		item->SetShortcut('C', B_COMMAND_KEY);
		item->Message()->what = B_COPY;
	}
}


void
TFileMixin::UpdatePasteMenuItem(BMenuItem* item)
{
	if (item == NULL || item->Menu() == NULL || item->Message() == NULL)
		return;

	if ((modifiers() & B_SHIFT_KEY) != 0) {
		item->SetLabel(B_TRANSLATE("Paste links"));
		item->SetShortcut('V', B_COMMAND_KEY | B_SHIFT_KEY);
		item->Message()->what = kPasteLinksFromClipboard;
	} else {
		item->SetLabel(B_TRANSLATE("Paste"));
		item->SetShortcut('V', B_COMMAND_KEY);
		item->Message()->what = B_PASTE;
	}
}


void
TFileMixin::UpdateIdentifyMenuItem(BMenuItem* item)
{
	if (item == NULL || item->Menu() == NULL || item->Message() == NULL)
		return;

	if ((modifiers() & B_SHIFT_KEY) != 0) {
		item->SetLabel(B_TRANSLATE("Force identify"));
		item->Message()->ReplaceBool("force", true);
	} else {
		item->SetLabel(B_TRANSLATE("Identify"));
		item->Message()->ReplaceBool("force", false);
	}
}


//	#pragma mark - TLiveUpdatingFileMenu


TLiveUpdatingFileMenu::TLiveUpdatingFileMenu(const char* label)
	:
	TLiveUpdatingMenu(label)
{
}


TLiveUpdatingFileMenu::~TLiveUpdatingFileMenu()
{
}


void
TLiveUpdatingFileMenu::LayoutChanged()
{
	BMenuWindow* menuWindow = dynamic_cast<BMenuWindow*>(Window());
	if (menuWindow != NULL)
		menuWindow->ResizeTo(Frame().Width(), menuWindow->Bounds().Height());

	BMenu::LayoutChanged();
}


void
TLiveUpdatingFileMenu::UpdateMenuItemsForModifiersChanged()
{
	if (Window()->LockLooper()) {
		// Create link/Create relative link
		UpdateCreateLinkMenuItemsInMenu(this);

		// Cut/Cut more
		BMenuItem* cut = FindItem(B_CUT);
		if (cut == NULL)
			cut = FindItem(kCutMoreSelectionToClipboard);
		UpdateCutMenuItem(cut);

		// Copy/Copy more
		BMenuItem* copy = FindItem(B_COPY);
		if (copy == NULL)
			copy = FindItem(kCopyMoreSelectionToClipboard);
		UpdateCopyMenuItem(copy);

		// Paste/Paste links
		BMenuItem* paste = FindItem(B_PASTE);
		if (paste == NULL)
			paste = FindItem(kPasteLinksFromClipboard);
		UpdatePasteMenuItem(paste);

		// Identify/Force identify
		UpdateIdentifyMenuItem(FindItem(kIdentifyEntry));

		Window()->UnlockLooper();
	}
}


//	#pragma mark - TLiveUpdatingFilePopUpMenu


TLiveUpdatingFilePopUpMenu::TLiveUpdatingFilePopUpMenu(const char* label,
	bool radioMode = true, bool labelFromMarked, menu_layout layout)
	:
	TLiveUpdatingPopUpMenu(label, radioMode, labelFromMarked, layout)
{
}


TLiveUpdatingFilePopUpMenu::~TLiveUpdatingFilePopUpMenu()
{
}


void
TLiveUpdatingFilePopUpMenu::LayoutChanged()
{
	BMenuWindow* menuWindow = dynamic_cast<BMenuWindow*>(Window());
	if (menuWindow != NULL)
		menuWindow->ResizeTo(Frame().Width(), menuWindow->Bounds().Height());

	BPopUpMenu::LayoutChanged();
}


void
TLiveUpdatingFilePopUpMenu::UpdateMenuItemsForModifiersChanged()
{
	if (Window()->LockLooper()) {
		// Create link/Create relative link
		UpdateCreateLinkMenuItemsInMenu(this);

		// Cut/Cut more
		BMenuItem* cut = FindItem(B_CUT);
		if (cut == NULL)
			cut = FindItem(kCutMoreSelectionToClipboard);
		UpdateCutMenuItem(cut);

		// Copy/Copy more
		BMenuItem* copy = FindItem(B_COPY);
		if (copy == NULL)
			copy = FindItem(kCopyMoreSelectionToClipboard);
		UpdateCopyMenuItem(copy);

		// Paste/Paste links
		BMenuItem* paste = FindItem(B_PASTE);
		if (paste == NULL)
			paste = FindItem(kPasteLinksFromClipboard);
		UpdatePasteMenuItem(paste);

		// Identify/Force identify
		UpdateIdentifyMenuItem(FindItem(kIdentifyEntry));

		Window()->UnlockLooper();
	}
}


//	#pragma mark - TLiveUpdatingWindowMenu


TLiveUpdatingWindowMenu::TLiveUpdatingWindowMenu(const char* label)
	:
	TLiveUpdatingMenu(label)
{
}


TLiveUpdatingWindowMenu::~TLiveUpdatingWindowMenu()
{
}


void
TLiveUpdatingWindowMenu::LayoutChanged()
{
	BMenuWindow* menuWindow = dynamic_cast<BMenuWindow*>(Window());
	if (menuWindow != NULL)
		menuWindow->ResizeTo(Frame().Width(), menuWindow->Bounds().Height());

	BMenu::LayoutChanged();
}


void
TLiveUpdatingWindowMenu::UpdateMenuItemsForModifiersChanged()
{
	if (Window()->LockLooper()) {
		// Close/Close all
		BMenuItem* close = FindItem(B_QUIT_REQUESTED);
		if (close == NULL)
			close = FindItem(kCloseAllWindows);
		UpdateCloseMenuItem(close);

		Window()->UnlockLooper();
	}
}


void
TLiveUpdatingWindowMenu::UpdateCloseMenuItem(BMenuItem* item)
{
	if (item == NULL || item->Menu() == NULL || item->Message() == NULL)
		return;

	if ((modifiers() & B_SHIFT_KEY) != 0) {
		item->SetLabel(B_TRANSLATE("Close all"));
		item->SetShortcut('W', B_COMMAND_KEY | B_SHIFT_KEY);
		item->SetTarget(be_app);
		item->Message()->what = kCloseAllWindows;
	} else {
		item->SetLabel(B_TRANSLATE("Close"));
		item->SetShortcut('W', B_COMMAND_KEY);
		item->SetTarget(Window());
		item->Message()->what = B_QUIT_REQUESTED;
	}
}
