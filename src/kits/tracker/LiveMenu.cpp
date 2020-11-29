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
#include <Locale.h>
#include <MenuItem.h>
#include <Window.h>

#include "Commands.h"
#include "Shortcuts.h"
#include "TrackerSettings.h"


//	#pragma mark - TLiveMixin


void
TLiveMixin::UpdateFileMenu(BMenu* menu)
{
	if (menu == NULL)
		return;

	if (menu->Window()->LockLooper()) {
		// Create link/Create relative link
		TShortcuts().UpdateCreateLinkItem(menu);

		// Cut/Cut more
		TShortcuts().UpdateCutItem(menu);

		// Copy/Copy more
		TShortcuts().UpdateCopyItem(menu);

		// Paste/Paste links
		TShortcuts().UpdatePasteItem(menu);

		// Identify/Force identify
		TShortcuts().UpdateIdentifyItem(menu);

		// Move to Trash/Delete
		TShortcuts().UpdateMoveToTrashItem(menu);

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
		TShortcuts().UpdateCleanupItem(menu);

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


//	#pragma mark - TLivePosePopUpMenu


TLivePosePopUpMenu::TLivePosePopUpMenu(const char* label,
	bool radioMode, bool labelFromMarked, menu_layout layout)
	:
	TLivePopUpMenu(label, radioMode, labelFromMarked, layout)
{
}


TLivePosePopUpMenu::~TLivePosePopUpMenu()
{
}


void
TLivePosePopUpMenu::Update()
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
