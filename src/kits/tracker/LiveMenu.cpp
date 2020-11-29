/*
 * Copyright 2020-2023 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */


#include "LiveMenu.h"

#include <string.h>

#include <Application.h>
#include <Catalog.h>
#include <Locale.h>
#include <MenuItem.h>
#include <Window.h>

#include "Commands.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ContainerWindow"


//	#pragma mark - TLiveMixin


void
TLiveMixin::UpdateCleanUpMenuItem(BMenuItem* item)
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


void
TLiveMixin::UpdateCreateLinkMenuItem(BMenuItem* item)
{
	if (item == NULL || item->Menu() == NULL || item->Message() == NULL)
		return;

	if ((modifiers() & B_SHIFT_KEY) != 0) {
		item->SetLabel(B_TRANSLATE("Create relative link"));
		item->SetMessage(new BMessage(kCreateRelativeLink));
	} else {
		item->SetLabel(B_TRANSLATE("Create link"));
		item->SetMessage(new BMessage(kCreateLink));
	}
}


void
TLiveMixin::UpdateCreateLinkMenuItemsInMenu(BMenu* menu)
{
	for (int32 i = menu->CountItems(); i-- > 0;) {
		BMenuItem* item = menu->ItemAt(i);
		if (item == NULL || item->Message() == NULL)
			continue; // invalid

		uint32 what = item->Message()->what;
		if (what != kCreateLink && what != kCreateRelativeLink)
			continue; // not our menu item

		const char* label = item->Label();
		if (strcmp(label, B_TRANSLATE("Create link")) == 0
			|| strcmp(label, B_TRANSLATE("Create relative link")) == 0) {
			// only update label if "Create link" or "Create relative link"
			UpdateCreateLinkMenuItem(item);
		} else {
			// only update the item message command
			item->Message()->what = ((modifiers() & B_SHIFT_KEY) != 0
				? kCreateRelativeLink : kCreateLink);
		}
	}
}


void
TLiveMixin::UpdateCutMenuItem(BMenuItem* item)
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
TLiveMixin::UpdateCopyMenuItem(BMenuItem* item)
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
TLiveMixin::UpdatePasteMenuItem(BMenuItem* item)
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
TLiveMixin::UpdateIdentifyMenuItem(BMenuItem* item)
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


//	#pragma mark - TLiveMenu


TLiveMenu::TLiveMenu(const char* label)
	:
	BMenu(label)
{
}


TLiveMenu::~TLiveMenu()
{
}


void
TLiveMenu::MessageReceived(BMessage* message)
{
	if (message != NULL && message->what == B_MODIFIERS_CHANGED) {
		Update();
	} else
		BMenu::MessageReceived(message);
}


void
TLiveMenu::Update()
{
	// hook method
}


//	#pragma mark - TLivePopUpMenu


TLivePopUpMenu::TLivePopUpMenu(const char* label,
	bool radioMode, bool labelFromMarked, menu_layout layout)
	:
	BPopUpMenu(label, radioMode, labelFromMarked, layout)
{
}


TLivePopUpMenu::~TLivePopUpMenu()
{
}


void
TLivePopUpMenu::MessageReceived(BMessage* message)
{
	if (message != NULL && message->what == B_MODIFIERS_CHANGED)
		Update();
	else
		BMenu::MessageReceived(message);
}


void
TLivePopUpMenu::Update()
{
	// hook method
}


//	#pragma mark - TLiveArrangeByMenu


TLiveArrangeByMenu::TLiveArrangeByMenu(const char* label)
	:
	TLiveMenu(label)
{
}


TLiveArrangeByMenu::~TLiveArrangeByMenu()
{
}


void
TLiveArrangeByMenu::Update()
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


//	#pragma mark - TLiveFileMenu


TLiveFileMenu::TLiveFileMenu(const char* label)
	:
	TLiveMenu(label)
{
}


TLiveFileMenu::~TLiveFileMenu()
{
}


void
TLiveFileMenu::Update()
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


//	#pragma mark - TLiveFilePopUpMenu


TLiveFilePopUpMenu::TLiveFilePopUpMenu(const char* label,
	bool radioMode, bool labelFromMarked, menu_layout layout)
	:
	TLivePopUpMenu(label, radioMode, labelFromMarked, layout)
{
}


TLiveFilePopUpMenu::~TLiveFilePopUpMenu()
{
}


void
TLiveFilePopUpMenu::Update()
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


//	#pragma mark - TLiveWindowMenu


TLiveWindowMenu::TLiveWindowMenu(const char* label)
	:
	TLiveMenu(label)
{
}


TLiveWindowMenu::~TLiveWindowMenu()
{
}


void
TLiveWindowMenu::Update()
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
TLiveWindowMenu::UpdateCloseMenuItem(BMenuItem* item)
{
	if (item == NULL || item->Menu() == NULL || item->Message() == NULL)
		return;

	if ((modifiers() & B_SHIFT_KEY) != 0) {
		item->SetLabel(B_TRANSLATE("Close all"));
		item->SetShortcut('W', B_COMMAND_KEY | B_SHIFT_KEY);
		item->SetTarget(be_app);
		item->SetMessage(new BMessage(kCloseAllWindows));
	} else {
		item->SetLabel(B_TRANSLATE("Close"));
		item->SetShortcut('W', B_COMMAND_KEY);
		item->SetTarget(Window());
		item->SetMessage(new BMessage(B_QUIT_REQUESTED));
	}
}


//	#pragma mark - TLiveWindowPopUpMenu


TLiveWindowPopUpMenu::TLiveWindowPopUpMenu(const char* label,
	bool radioMode, bool labelFromMarked, menu_layout layout)
	:
	TLivePopUpMenu(label, radioMode, labelFromMarked, layout)
{
}


TLiveWindowPopUpMenu::~TLiveWindowPopUpMenu()
{
}


void
TLiveWindowPopUpMenu::Update()
{
	if (Window()->LockLooper()) {
		// Clean up/Clean up all (on Desktop)
		BMenuItem* cleanUp = FindItem(kCleanup);
		if (cleanUp == NULL)
			cleanUp = FindItem(kCleanupAll);
		UpdateCleanUpMenuItem(cleanUp);

		Window()->UnlockLooper();
	}
}
