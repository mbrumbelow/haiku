/*
 * Copyright 2020-2023 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */


#include "Shortcuts.h"

#include <Catalog.h>
#include <InterfaceDefs.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Message.h>

#include "Commands.h"
#include "TrackerSettings.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ContainerWindow"


Shortcuts::Shortcuts()
{
}


BMenuItem*
Shortcuts::AddPrinterItem()
{
	return new BMenuItem(AddPrinterLabel(), new BMessage(kAddPrinter));
}


const char*
Shortcuts::AddPrinterLabel()
{
	return B_TRANSLATE("Add printer" B_UTF8_ELLIPSIS);
}


BMenuItem*
Shortcuts::CleanupItem()
{
	return new BMenuItem(B_TRANSLATE("Clean up"), new BMessage(kCleanup), 'K');
}


const char*
Shortcuts::CleanupLabel()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? B_TRANSLATE("Clean up all")
		: B_TRANSLATE("Clean up");
}


int32
Shortcuts::CleanupCommand()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? kCleanupAll
		: kCleanup;
}


BMenuItem*
Shortcuts::CloseItem()
{
	return new BMenuItem(B_TRANSLATE("Close"), new BMessage(B_QUIT_REQUESTED), 'W');
}


const char*
Shortcuts::CloseLabel()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? B_TRANSLATE("Close all")
		: B_TRANSLATE("Close");
}


int32
Shortcuts::CloseCommand()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? kCloseAllWindows
		: B_QUIT_REQUESTED;
}


BMenuItem*
Shortcuts::CloseAllInWorkspaceItem()
{
	return new BMenuItem(CloseAllInWorkspaceLabel(), new BMessage(kCloseAllInWorkspace), 'Q');
}


const char*
Shortcuts::CloseAllInWorkspaceLabel()
{
	return B_TRANSLATE("Close all in workspace");
}


BMenuItem*
Shortcuts::CopyItem()
{
	return new BMenuItem(B_TRANSLATE("Copy"), new BMessage(B_COPY), 'C');
}


const char*
Shortcuts::CopyLabel()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? B_TRANSLATE("Copy more")
		: B_TRANSLATE("Copy");
}


int32
Shortcuts::CopyCommand()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? kCopyMoreSelectionToClipboard
		: B_COPY;
}


BMenuItem*
Shortcuts::CopyToItem(BMenu* menu)
{
	return new BMenuItem(menu, new BMessage(kCopySelectionTo));
}


const char*
Shortcuts::CopyToLabel()
{
	return B_TRANSLATE("Copy to");
}


BMenuItem*
Shortcuts::CreateLinkItem(BMenu* menu)
{
	return new BMenuItem(menu, new BMessage(kCreateLink));
}


const char*
Shortcuts::CreateLinkLabel()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? B_TRANSLATE("Create relative link")
		: B_TRANSLATE("Create link");
}


int32
Shortcuts::CreateLinkCommand()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? kCreateRelativeLink
		: kCreateLink;
}


BMenuItem*
Shortcuts::CreateLinkHereItem()
{
	return new BMenuItem(B_TRANSLATE("Create link here"), new BMessage(kCreateLink));
}


const char*
Shortcuts::CreateLinkHereLabel()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? B_TRANSLATE("Create relative link here")
		: B_TRANSLATE("Create link here");
}


int32
Shortcuts::CreateLinkHereCommand()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? kCreateRelativeLink
		: kCreateLink;
}


BMenuItem*
Shortcuts::CutItem()
{
	return new BMenuItem(CutLabel(), new BMessage(B_CUT), 'X');
}


const char*
Shortcuts::CutLabel()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? B_TRANSLATE("Cut more")
		: B_TRANSLATE("Cut");
}


int32
Shortcuts::CutCommand()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? kCutMoreSelectionToClipboard
		: B_CUT;
}


BMenuItem*
Shortcuts::DeleteItem()
{
	return new BMenuItem(DeleteLabel(), new BMessage(kDelete));
}


const char*
Shortcuts::DeleteLabel()
{
	return B_TRANSLATE("Delete");
}


BMenuItem*
Shortcuts::DuplicateItem()
{
	return new BMenuItem(DuplicateLabel(), new BMessage(kDuplicateSelection), 'D');
}


const char*
Shortcuts::DuplicateLabel()
{
	return B_TRANSLATE("Duplicate");
}


BMenuItem*
Shortcuts::EditNameItem()
{
	return new BMenuItem(EditNameLabel(), new BMessage(kEditItem), 'E');
}


const char*
Shortcuts::EditNameLabel()
{
	return B_TRANSLATE("Edit name");
}


BMenuItem*
Shortcuts::EditQueryItem()
{
	return new BMenuItem(EditQueryLabel(), new BMessage(kEditQuery), 'G');
}


const char*
Shortcuts::EditQueryLabel()
{
	return B_TRANSLATE("Edit query");
}


BMenuItem*
Shortcuts::EmptyTrashItem()
{
	return new BMenuItem(EmptyTrashLabel(), new BMessage(kEmptyTrash));
}


const char*
Shortcuts::EmptyTrashLabel()
{
	return B_TRANSLATE("Empty Trash");
}


BMenuItem*
Shortcuts::FindItem()
{
	return new BMenuItem(FindLabel(), new BMessage(kFindButton), 'F');
}


const char*
Shortcuts::FindLabel()
{
	return B_TRANSLATE("Find" B_UTF8_ELLIPSIS);
}


BMenuItem*
Shortcuts::GetInfoItem()
{
	return new BMenuItem(GetInfoLabel(), new BMessage(kGetInfo), 'I');
}


const char*
Shortcuts::GetInfoLabel()
{
	return B_TRANSLATE("Get info");
}


BMenuItem*
Shortcuts::IdentifyItem()
{
	BMessage* message = new BMessage(kIdentifyEntry);
	BMenuItem* item = new BMenuItem(IdentifyLabel(), message);
	message->AddBool("force", false);
	return item;
}


const char*
Shortcuts::IdentifyLabel()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? B_TRANSLATE("Force identify")
		: B_TRANSLATE("Identify");
}


BMenuItem*
Shortcuts::InvertSelectionItem()
{
	return new BMenuItem(InvertSelectionLabel(), new BMessage(kInvertSelection), 'S');
}


const char*
Shortcuts::InvertSelectionLabel()
{
	return B_TRANSLATE("Invert selection");
}


BMenuItem*
Shortcuts::MakeActivePrinterItem()
{
	return new BMenuItem(MakeActivePrinterLabel(), new BMessage(kMakeActivePrinter));
}


const char*
Shortcuts::MakeActivePrinterLabel()
{
	return B_TRANSLATE("Make active printer");
}


const char*
Shortcuts::MountLabel()
{
	return B_TRANSLATE("Mount");
}


BMenuItem*
Shortcuts::MoveToItem(BMenu* menu)
{
	return new BMenuItem(menu, new BMessage(kMoveSelectionTo));
}


const char*
Shortcuts::MoveToLabel()
{
	return B_TRANSLATE("Move to");
}


BMenuItem*
Shortcuts::MoveToTrashItem()
{
	return new BMenuItem(B_TRANSLATE("Move to Trash"), new BMessage(kMoveToTrash), 'T');
}


const char*
Shortcuts::MoveToTrashLabel()
{
	return TrackerSettings().DontMoveFilesToTrash() || (modifiers() & B_SHIFT_KEY) != 0
		? B_TRANSLATE("Delete")
		: B_TRANSLATE("Move to Trash");
}


int32
Shortcuts::MoveToTrashCommand()
{
	return TrackerSettings().DontMoveFilesToTrash() || (modifiers() & B_SHIFT_KEY) != 0
		? kDelete
		: kMoveToTrash;
}


BMenuItem*
Shortcuts::NewFolderItem()
{
	return new BMenuItem(NewFolderLabel(), new BMessage(kNewFolder), 'N');
}


const char*
Shortcuts::NewFolderLabel()
{
	return B_TRANSLATE("New folder");
}


BMenuItem*
Shortcuts::OpenItem()
{
	return new BMenuItem(OpenLabel(), new BMessage(kOpenSelection), 'O');
}


const char*
Shortcuts::OpenLabel()
{
	return B_TRANSLATE("Open");
}


BMenuItem*
Shortcuts::OpenParentItem()
{
	return new BMenuItem(OpenParentLabel(), new BMessage(kOpenParentDir), B_UP_ARROW);
}


const char*
Shortcuts::OpenParentLabel()
{
	return B_TRANSLATE("Open parent");
}


BMenuItem*
Shortcuts::OpenWithItem()
{
	return new BMenuItem(OpenWithhLabel(), new BMessage(kOpenSelectionWith));
}


BMenuItem*
Shortcuts::OpenWithItem(BMenu* menu)
{
	return new BMenuItem(menu, new BMessage(kOpenSelectionWith));
}


const char*
Shortcuts::OpenWithhLabel()
{
	return B_TRANSLATE("Open with" B_UTF8_ELLIPSIS);
}


BMenuItem*
Shortcuts::PasteItem()
{
	return new BMenuItem(PasteLabel(), new BMessage(B_PASTE), 'V');
}


const char*
Shortcuts::PasteLabel()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? B_TRANSLATE("Paste links")
		: B_TRANSLATE("Paste");
}


int32
Shortcuts::PasteCommand()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? kPasteLinksFromClipboard
		: B_PASTE;
}


BMenuItem*
Shortcuts::RestoreItem()
{
	return new BMenuItem(RestoreLabel(), new BMessage(kRestoreFromTrash));
}


const char*
Shortcuts::RestoreLabel()
{
	return B_TRANSLATE("Restore");
}


BMenuItem*
Shortcuts::ReverseOrderItem()
{
	return new BMenuItem(ReverseOrderLabel(), new BMessage(kArrangeReverseOrder));
}


const char*
Shortcuts::ReverseOrderLabel()
{
	return B_TRANSLATE("Reverse order");
}


BMenuItem*
Shortcuts::ResizeToFitItem()
{
	return new BMenuItem(ResizeToFitLabel(), new BMessage(kResizeToFit), 'Y');
}


const char*
Shortcuts::ResizeToFitLabel()
{
	return B_TRANSLATE("Resize to fit");
}



BMenuItem*
Shortcuts::SelectItem()
{
	return new BMenuItem(SelectLabel(), new BMessage(kShowSelectionWindow), 'A', B_SHIFT_KEY);
}


const char*
Shortcuts::SelectLabel()
{
	return B_TRANSLATE("Select" B_UTF8_ELLIPSIS);
}


BMenuItem*
Shortcuts::SelectAllItem()
{
	return new BMenuItem(SelectAllLabel(), new BMessage(B_SELECT_ALL), 'A', B_SHIFT_KEY);
}


const char*
Shortcuts::SelectAllLabel()
{
	return B_TRANSLATE("Select all");
}


BMenuItem*
Shortcuts::UnmountItem()
{
	return new BMenuItem(UnmountLabel(), new BMessage(kUnmountVolume), 'U');
}


const char*
Shortcuts::UnmountLabel()
{
	return B_TRANSLATE("Unmount");
}


BMenuItem*
Shortcuts::UnmountAllItem()
{
	return new BMenuItem(UnmountAllLabel(), new BMessage(kUnmountAllVolumes), 'U', B_SHIFT_KEY);
}


const char*
Shortcuts::UnmountAllLabel()
{
	return B_TRANSLATE("Unmount All");
}


void
Shortcuts::UpdateCleanupItem(BMenuItem* item)
{
	item->SetLabel(CleanupLabel());
	item->Message()->what = CleanupCommand();
	item->SetShortcut(item->Shortcut(), B_COMMAND_KEY | (modifiers() & B_SHIFT_KEY));
}


void
Shortcuts::UpdateCloseItem(BMenuItem* item)
{
	item->SetLabel(CloseLabel());
	item->Message()->what = CloseCommand();
	item->SetShortcut(item->Shortcut(), B_COMMAND_KEY | (modifiers() & B_SHIFT_KEY));
}


void
Shortcuts::UpdateCopyItem(BMenuItem* item)
{
	item->SetLabel(CopyLabel());
	item->Message()->what = CopyCommand();
	item->SetShortcut(item->Shortcut(), B_COMMAND_KEY | (modifiers() & B_SHIFT_KEY));
}


void
Shortcuts::UpdateCreateLinkItem(BMenuItem* item)
{
	item->SetLabel(CreateLinkLabel());
	item->Message()->what = CreateLinkCommand();
}


void
Shortcuts::UpdateCreateLinkHereItem(BMenuItem* item)
{
	item->SetLabel(CreateLinkHereLabel());
	item->Message()->what = CreateLinkHereCommand();
}


void
Shortcuts::UpdateCreateLinkItemsInMenu(BMenu* menu)
{
	int32 itemCount = menu->CountItems();
	for (int32 index = 0; index < itemCount; index++) {
		BMenuItem* item = menu->ItemAt(index);
		if (item == NULL || item->Message() == NULL)
			continue; // invalid

		uint32 command = item->Message()->what;
		if (command == kCreateLink || command == kCreateRelativeLink)
			UpdateCreateLinkItem(item);
	}
}


void
Shortcuts::UpdateCutItem(BMenuItem* item)
{
	item->SetLabel(CutLabel());
	item->Message()->what = CutCommand();
	item->SetShortcut(item->Shortcut(), B_COMMAND_KEY | (modifiers() & B_SHIFT_KEY));
}


void
Shortcuts::UpdateIdentifyItem(BMenuItem* item)
{
	item->SetLabel(IdentifyLabel());
	item->Message()->ReplaceBool("force", (modifiers() & B_SHIFT_KEY) != 0);
}


void
Shortcuts::UpdateMoveToTrashItem(BMenuItem* item)
{
	item->SetLabel(MoveToTrashLabel());
	item->Message()->what = MoveToTrashCommand();
	if (TrackerSettings().DontMoveFilesToTrash())
		item->SetShortcut(item->Shortcut(), B_COMMAND_KEY);
	else
		item->SetShortcut(item->Shortcut(), B_COMMAND_KEY | (modifiers() & B_SHIFT_KEY));
}


void
Shortcuts::UpdatePasteItem(BMenuItem* item)
{
	item->SetLabel(PasteLabel());
	item->Message()->what = PasteCommand();
	item->SetShortcut(item->Shortcut(), B_COMMAND_KEY | (modifiers() & B_SHIFT_KEY));
}
