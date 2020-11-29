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
#include "Shortcuts.h"
#include "TrackerSettings.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ContainerWindow"


//	#pragma mark - TLiveMixin


void
TLiveMixin::UpdateFileMenu(BMenu* menu)
{
	if (menu == NULL)
		return;

	if (menu->Window()->LockLooper()) {
		// Create link/Create relative link
		Shortcuts().UpdateCreateLinkInMenu(menu);

		// Cut/Cut more
		BMenuItem* cut = menu->FindItem(B_CUT);
		if (cut == NULL)
			cut = menu->FindItem(kCutMoreSelectionToClipboard);
		if (cut != NULL)
			Shortcuts().UpdateCutItem(cut);

		// Copy/Copy more
		BMenuItem* copy = menu->FindItem(B_COPY);
		if (copy == NULL)
			copy = menu->FindItem(kCopyMoreSelectionToClipboard);
		if (copy != NULL)
			Shortcuts().UpdateCopyItem(copy);

		// Paste/Paste links
		BMenuItem* paste = menu->FindItem(B_PASTE);
		if (paste == NULL)
			paste = menu->FindItem(kPasteLinksFromClipboard);
		if (paste != NULL)
			Shortcuts().UpdatePasteItem(paste);

		// Identify/Force identify
		Shortcuts().UpdateIdentifyItem(menu->FindItem(kIdentifyEntry));

		// Move to Trash/Delete
		BMenuItem* trash = menu->FindItem(kMoveToTrash);
		if (trash == NULL)
			trash = menu->FindItem(kDelete);
		if (trash != NULL)
			Shortcuts().UpdateMoveToTrashItem(trash);

		menu->Window()->UnlockLooper();
	}
}


void
TLiveMixin::UpdateWindowMenu(BMenu* menu)
{
	if (menu == NULL)
		return;

	if (menu->Window()->LockLooper()) {
		// Clean up/Clean up all
		BMenuItem* cleanup = menu->FindItem(kCleanup);
		if (cleanup == NULL)
			cleanup = menu->FindItem(kCleanupAll);
		Shortcuts().UpdateCleanupItem(cleanup);

		menu->Window()->UnlockLooper();
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
	if (message != NULL && message->what == B_MODIFIERS_CHANGED)
		Update();
	else
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
	UpdateWindowMenu(this);
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
	UpdateFileMenu(this);
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
	UpdateFileMenu(this);
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
	UpdateWindowMenu(this);
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
	UpdateWindowMenu(this);
}
