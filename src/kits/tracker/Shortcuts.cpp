/*
 * Copyright 2020-2023 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */


#include "Shortcuts.h"

#include <Application.h>
#include <Catalog.h>
#include <InterfaceDefs.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Message.h>
#include <Messenger.h>
#include <TextView.h>
#include <View.h>
#include <Window.h>

#include "Commands.h"
#include "FSClipboard.h"
#include "FSUtils.h"
#include "Pose.h"
#include "Model.h"
#include "Utilities.h"
#include "Tracker.h"
#include "TrackerSettings.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ContainerWindow"


TShortcuts::TShortcuts()
	:
	fContainerWindow(NULL),
	fInWindow(false)
{
	// the dumb version: for adding and limited updates
}


TShortcuts::TShortcuts(BContainerWindow* window)
	:
	fContainerWindow(window),
	fInWindow(window != NULL)
{
	// the smart version: update methods work
}


//	#pragma mark - Shortcuts build methods


BMenuItem*
TShortcuts::AddOnsItem()
{
	return new BMenuItem(B_TRANSLATE("Add-ons"), NULL);
}


BMenuItem*
TShortcuts::AddOnsItem(BMenu* menu)
{
	return new BMenuItem(menu);
}


const char*
TShortcuts::AddOnsLabel()
{
	return B_TRANSLATE("Add-ons");
}


BMenuItem*
TShortcuts::AddPrinterItem()
{
	return new BMenuItem(B_TRANSLATE("Add printer" B_UTF8_ELLIPSIS), new BMessage(kAddPrinter));
}


const char*
TShortcuts::AddPrinterLabel()
{
	return B_TRANSLATE("Add printer" B_UTF8_ELLIPSIS);
}


BMenuItem*
TShortcuts::CleanupItem()
{
	return new BMenuItem(B_TRANSLATE("Clean up"), new BMessage(kCleanup), 'K');
}


const char*
TShortcuts::CleanupLabel()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? B_TRANSLATE("Clean up all")
		: B_TRANSLATE("Clean up");
}


int32
TShortcuts::CleanupCommand()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? kCleanupAll
		: kCleanup;
}


BMenuItem*
TShortcuts::CloseItem()
{
	return new BMenuItem(B_TRANSLATE("Close"), new BMessage(B_QUIT_REQUESTED), 'W');
}


const char*
TShortcuts::CloseLabel()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? B_TRANSLATE("Close all")
		: B_TRANSLATE("Close");
}


int32
TShortcuts::CloseCommand()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? kCloseAllWindows
		: B_QUIT_REQUESTED;
}


BMenuItem*
TShortcuts::CloseAllInWorkspaceItem()
{
	return new BMenuItem(B_TRANSLATE("Close all in workspace"), new BMessage(kCloseAllInWorkspace),
		'Q');
}


const char*
TShortcuts::CloseAllInWorkspaceLabel()
{
	return B_TRANSLATE("Close all in workspace");
}


BMenuItem*
TShortcuts::CopyItem()
{
	return new BMenuItem(B_TRANSLATE("Copy"), new BMessage(B_COPY), 'C');
}


const char*
TShortcuts::CopyLabel()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? B_TRANSLATE("Copy more")
		: B_TRANSLATE("Copy");
}


int32
TShortcuts::CopyCommand()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? kCopyMoreSelectionToClipboard
		: B_COPY;
}


BMenuItem*
TShortcuts::CopyToItem()
{
	return new BMenuItem(B_TRANSLATE("Copy to"), new BMessage(kCopySelectionTo));
}


BMenuItem*
TShortcuts::CopyToItem(BMenu* menu)
{
	return new BMenuItem(menu, new BMessage(kCopySelectionTo));
}


const char*
TShortcuts::CopyToLabel()
{
	return B_TRANSLATE("Copy to");
}


BMenuItem*
TShortcuts::CreateLinkItem()
{
	return new BMenuItem(B_TRANSLATE("Create link"), new BMessage(kCreateLink));
}


BMenuItem*
TShortcuts::CreateLinkItem(BMenu* menu)
{
	return new BMenuItem(menu, new BMessage(kCreateLink));
}


const char*
TShortcuts::CreateLinkLabel()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? B_TRANSLATE("Create relative link")
		: B_TRANSLATE("Create link");
}


int32
TShortcuts::CreateLinkCommand()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? kCreateRelativeLink
		: kCreateLink;
}


BMenuItem*
TShortcuts::CreateLinkHereItem()
{
	return new BMenuItem(B_TRANSLATE("Create link here"), new BMessage(kCreateLink));
}


const char*
TShortcuts::CreateLinkHereLabel()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? B_TRANSLATE("Create relative link here")
		: B_TRANSLATE("Create link here");
}


int32
TShortcuts::CreateLinkHereCommand()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? kCreateRelativeLink
		: kCreateLink;
}


BMenuItem*
TShortcuts::CutItem()
{
	return new BMenuItem(B_TRANSLATE("Cut"), new BMessage(B_CUT), 'X');
}


const char*
TShortcuts::CutLabel()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? B_TRANSLATE("Cut more")
		: B_TRANSLATE("Cut");
}


int32
TShortcuts::CutCommand()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? kCutMoreSelectionToClipboard
		: B_CUT;
}


BMenuItem*
TShortcuts::DeleteItem()
{
	return new BMenuItem(B_TRANSLATE("Delete"), new BMessage(kDeleteSelection));
}


const char*
TShortcuts::DeleteLabel()
{
	return B_TRANSLATE("Delete");
}


BMenuItem*
TShortcuts::DuplicateItem()
{
	return new BMenuItem(B_TRANSLATE("Duplicate"), new BMessage(kDuplicateSelection), 'D');
}


const char*
TShortcuts::DuplicateLabel()
{
	return B_TRANSLATE("Duplicate");
}


BMenuItem*
TShortcuts::EditNameItem()
{
	return new BMenuItem(B_TRANSLATE("Edit name"), new BMessage(kEditName), 'E');
}


const char*
TShortcuts::EditNameLabel()
{
	return B_TRANSLATE("Edit name");
}


BMenuItem*
TShortcuts::EditQueryItem()
{
	return new BMenuItem(B_TRANSLATE("Edit query"), new BMessage(kEditQuery), 'G');
}


const char*
TShortcuts::EditQueryLabel()
{
	return B_TRANSLATE("Edit query");
}


BMenuItem*
TShortcuts::EmptyTrashItem()
{
	return new BMenuItem(B_TRANSLATE("Empty Trash"), new BMessage(kEmptyTrash));
}


const char*
TShortcuts::EmptyTrashLabel()
{
	return B_TRANSLATE("Empty Trash");
}


BMenuItem*
TShortcuts::FindItem()
{
	return new BMenuItem(B_TRANSLATE("Find" B_UTF8_ELLIPSIS), new BMessage(kFindButton), 'F');
}


const char*
TShortcuts::FindLabel()
{
	return B_TRANSLATE("Find" B_UTF8_ELLIPSIS);
}


BMenuItem*
TShortcuts::GetInfoItem()
{
	return new BMenuItem(B_TRANSLATE("Get info"), new BMessage(kGetInfo), 'I');
}


const char*
TShortcuts::GetInfoLabel()
{
	return B_TRANSLATE("Get info");
}


BMenuItem*
TShortcuts::IdentifyItem()
{
	BMessage* message = new BMessage(kIdentifyEntry);
	BMenuItem* item = new BMenuItem(B_TRANSLATE("Identify"), message);
	message->AddBool("force", false);

	if (fInWindow)
		item->SetTarget(PoseView());

	return item;
}


const char*
TShortcuts::IdentifyLabel()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? B_TRANSLATE("Force identify")
		: B_TRANSLATE("Identify");
}


BMenuItem*
TShortcuts::InvertSelectionItem()
{
	return new BMenuItem(B_TRANSLATE("Invert selection"), new BMessage(kInvertSelection), 'S');
}


const char*
TShortcuts::InvertSelectionLabel()
{
	return B_TRANSLATE("Invert selection");
}


BMenuItem*
TShortcuts::MakeActivePrinterItem()
{
	return new BMenuItem(B_TRANSLATE("Make active printer"), new BMessage(kMakeActivePrinter));
}


const char*
TShortcuts::MakeActivePrinterLabel()
{
	return B_TRANSLATE("Make active printer");
}


BMenuItem*
TShortcuts::MountItem()
{
	return new BMenuItem(B_TRANSLATE("Mount"), new BMessage(kMountVolume));
}


BMenuItem*
TShortcuts::MountItem(BMenu* menu)
{
	return new BMenuItem(menu, new BMessage(kMountVolume));
}


const char*
TShortcuts::MountLabel()
{
	return B_TRANSLATE("Mount");
}


BMenuItem*
TShortcuts::MoveToItem()
{
	return new BMenuItem(B_TRANSLATE("Move to"), new BMessage(kMoveSelectionTo));
}


BMenuItem*
TShortcuts::MoveToItem(BMenu* menu)
{
	return new BMenuItem(menu, new BMessage(kMoveSelectionTo));
}


const char*
TShortcuts::MoveToLabel()
{
	return B_TRANSLATE("Move to");
}


BMenuItem*
TShortcuts::MoveToTrashItem()
{
	return new BMenuItem(B_TRANSLATE("Move to Trash"), new BMessage(kMoveSelectionToTrash), 'T');
}


const char*
TShortcuts::MoveToTrashLabel()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? B_TRANSLATE("Delete")
		: B_TRANSLATE("Move to Trash");
}


int32
TShortcuts::MoveToTrashCommand()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? kDeleteSelection
		: kMoveSelectionToTrash;
}


BMenuItem*
TShortcuts::NewFolderItem()
{
	return new BMenuItem(B_TRANSLATE("New folder"), new BMessage(kNewFolder), 'N');
}


const char*
TShortcuts::NewFolderLabel()
{
	return B_TRANSLATE("New folder");
}


BMenuItem*
TShortcuts::NewTemplatesItem()
{
	return new BMenuItem(B_TRANSLATE("New"), new BMessage(kNewEntryFromTemplate));
}


BMenuItem*
TShortcuts::NewTemplatesItem(BMenu* menu)
{
	return new BMenuItem(menu, new BMessage(kNewEntryFromTemplate));
}


const char*
TShortcuts::NewTemplatesLabel()
{
	return B_TRANSLATE("New");
}



BMenuItem*
TShortcuts::OpenItem()
{
	return new BMenuItem(B_TRANSLATE("Open"), new BMessage(kOpenSelection), 'O');
}


const char*
TShortcuts::OpenLabel()
{
	return B_TRANSLATE("Open");
}


BMenuItem*
TShortcuts::OpenParentItem()
{
	return new BMenuItem(B_TRANSLATE("Open parent"), new BMessage(kOpenParentDir), B_UP_ARROW);
}


const char*
TShortcuts::OpenParentLabel()
{
	return B_TRANSLATE("Open parent");
}


BMenuItem*
TShortcuts::OpenWithItem()
{
	return new BMenuItem(B_TRANSLATE("Open with" B_UTF8_ELLIPSIS),
		new BMessage(kOpenSelectionWith));
}


BMenuItem*
TShortcuts::OpenWithItem(BMenu* menu)
{
	return new BMenuItem(menu, new BMessage(kOpenSelectionWith));
}


const char*
TShortcuts::OpenWithLabel()
{
	return B_TRANSLATE("Open with" B_UTF8_ELLIPSIS);
}


BMenuItem*
TShortcuts::PasteItem()
{
	return new BMenuItem(B_TRANSLATE("Paste"), new BMessage(B_PASTE), 'V');
}


const char*
TShortcuts::PasteLabel()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? B_TRANSLATE("Paste links")
		: B_TRANSLATE("Paste");
}


int32
TShortcuts::PasteCommand()
{
	return (modifiers() & B_SHIFT_KEY) != 0
		? kPasteLinksFromClipboard
		: B_PASTE;
}


BMenuItem*
TShortcuts::ResizeToFitItem()
{
	return new BMenuItem(B_TRANSLATE("Resize to fit"), new BMessage(kResizeToFit), 'Y');
}


const char*
TShortcuts::ResizeToFitLabel()
{
	return B_TRANSLATE("Resize to fit");
}


BMenuItem*
TShortcuts::RestoreItem()
{
	return new BMenuItem(B_TRANSLATE("Restore"), new BMessage(kRestoreSelectionFromTrash));
}


const char*
TShortcuts::RestoreLabel()
{
	return B_TRANSLATE("Restore");
}


BMenuItem*
TShortcuts::ReverseOrderItem()
{
	return new BMenuItem(B_TRANSLATE("Reverse order"), new BMessage(kArrangeReverseOrder));
}


const char*
TShortcuts::ReverseOrderLabel()
{
	return B_TRANSLATE("Reverse order");
}


BMenuItem*
TShortcuts::SelectItem()
{
	return new BMenuItem(B_TRANSLATE("Select" B_UTF8_ELLIPSIS), new BMessage(kShowSelectionWindow),
		'A', B_SHIFT_KEY);
}


const char*
TShortcuts::SelectLabel()
{
	return B_TRANSLATE("Select" B_UTF8_ELLIPSIS);
}


BMenuItem*
TShortcuts::SelectAllItem()
{
	return new BMenuItem(B_TRANSLATE("Select all"), new BMessage(B_SELECT_ALL), 'A', B_SHIFT_KEY);
}


const char*
TShortcuts::SelectAllLabel()
{
	return B_TRANSLATE("Select all");
}


BMenuItem*
TShortcuts::UnmountItem()
{
	return new BMenuItem(B_TRANSLATE("Unmount"), new BMessage(kUnmountVolume), 'U');
}


const char*
TShortcuts::UnmountLabel()
{
	return B_TRANSLATE("Unmount");
}


BMenuItem*
TShortcuts::UnmountAllItem()
{
	return new BMenuItem(B_TRANSLATE("Unmount all"), new BMessage(kUnmountAllVolumes), 'U',
		B_SHIFT_KEY);
}


const char*
TShortcuts::UnmountAllLabel()
{
	return B_TRANSLATE("Unmount all");
}


//	#pragma mark - Shortcuts update methods


void
TShortcuts::UpdateAddPrinterItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	item->SetEnabled(true);
	item->SetTarget(be_app);
}


void
TShortcuts::UpdateCleanupItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	item->SetLabel(CleanupLabel());
	item->Message()->what = CleanupCommand();
	item->SetShortcut(item->Shortcut(), B_COMMAND_KEY | (modifiers() & B_SHIFT_KEY));

	if (fInWindow) {
		item->SetEnabled(true);
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateCloseAllInWorkspaceItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	item->SetLabel(CloseAllInWorkspaceLabel());
	item->Message()->what = kCloseAllInWorkspace;

	item->SetEnabled(true);
	item->SetTarget(be_app);
}


void
TShortcuts::UpdateCloseItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	item->SetLabel(CloseLabel());
	item->Message()->what = CloseCommand();
	item->SetShortcut(item->Shortcut(), B_COMMAND_KEY | (modifiers() & B_SHIFT_KEY));

	if (fInWindow) {
		if ((modifiers() & B_SHIFT_KEY) != 0)
			item->SetTarget(be_app);
		else
			item->SetTarget(fContainerWindow);
	}
}


void
TShortcuts::UpdateCopyItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	item->SetLabel(CopyLabel());
	item->Message()->what = CopyCommand();
	item->SetShortcut(item->Shortcut(), B_COMMAND_KEY | (modifiers() & B_SHIFT_KEY));

	if (fInWindow) {
		item->SetEnabled(IsCurrentFocusOnTextView() || HasSelection());
		item->SetTarget(fContainerWindow);
	}
}


void
TShortcuts::UpdateCopyToItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(HasSelection());
		item->SetTarget(fContainerWindow);
	}
}


void
TShortcuts::UpdateCreateLinkItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	item->SetLabel(CreateLinkLabel());
	item->Message()->what = CreateLinkCommand();

	if (fInWindow) {
		item->SetEnabled(HasSelection());
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateCreateLinkHereItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	item->SetLabel(CreateLinkHereLabel());
	item->Message()->what = CreateLinkHereCommand();

	if (fInWindow) {
		item->SetEnabled(HasSelection());
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateCutItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	item->SetLabel(CutLabel());
	item->Message()->what = CutCommand();
	item->SetShortcut(item->Shortcut(), B_COMMAND_KEY | (modifiers() & B_SHIFT_KEY));

	if (fInWindow) {
		if (IsCurrentFocusOnTextView())
			item->SetEnabled(true);
		else if (IsRoot() || IsTrash() || IsVirtualDirectory())
			item->SetEnabled(false);
		else
			item->SetEnabled(HasSelection() && TargetIsReadOnly() == false);

		item->SetTarget(fContainerWindow);
	}
}


void
TShortcuts::UpdateDeleteItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(HasSelection());
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateDuplicateItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(PoseView()->CanMoveToTrashOrDuplicate());
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateEditNameItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(PoseView()->CanEditName());
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateEditQueryItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(HasSelection());
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateEmptyTrashItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(static_cast<TTracker*>(be_app)->TrashFull());
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateFindItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(true);
		item->SetTarget(fContainerWindow);
	}
}


void
TShortcuts::UpdateGetInfoItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(true);
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateIdentifyItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	item->SetLabel(IdentifyLabel());
	item->Message()->ReplaceBool("force", (modifiers() & B_SHIFT_KEY) != 0);

	if (fInWindow) {
		item->SetEnabled(HasSelection());
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateInvertSelectionItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(true);
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateMakeActivePrinterItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(PoseView()->CountSelected() == 1);
		item->SetTarget(be_app);
	}
}


void
TShortcuts::UpdateMoveToItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(HasSelection() && !PoseView()->SelectedVolumeIsReadOnly());
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateMoveToTrashItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (TrackerSettings().SkipTrash()) {
		item->SetLabel(B_TRANSLATE("Delete"));
		item->Message()->what = kDeleteSelection;
		item->SetShortcut(item->Shortcut(), B_COMMAND_KEY);
	} else {
		item->SetLabel(MoveToTrashLabel());
		item->Message()->what = MoveToTrashCommand();
		item->SetShortcut(item->Shortcut(), B_COMMAND_KEY | (modifiers() & B_SHIFT_KEY));
	}

	if (fInWindow) {
		item->SetEnabled(HasSelection() && !PoseView()->SelectedVolumeIsReadOnly());
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateNewFolderItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(TargetIsReadOnly() == false);
		item->SetTarget(fContainerWindow);
	}
}


void
TShortcuts::UpdateNewTemplatesItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow && item->Menu() != NULL) {
		item->SetEnabled(TargetIsReadOnly() == false);
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateOpenItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(HasSelection());
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateOpenParentItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled((!IsDesktop() && !IsRoot() && !ParentIsRoot())
			&& (TrackerSettings().SingleWindowBrowse() || TrackerSettings().ShowDisksIcon()
				|| (modifiers() & B_CONTROL_KEY) != 0));
		item->SetTarget(fContainerWindow);
	}
}


void
TShortcuts::UpdateOpenWithItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	item->SetShortcut('O', B_COMMAND_KEY | B_CONTROL_KEY);

	if (fInWindow) {
		item->SetEnabled(HasSelection());
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdatePasteItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	item->SetLabel(PasteLabel());
	item->Message()->what = PasteCommand();
	item->SetShortcut(item->Shortcut(), B_COMMAND_KEY | (modifiers() & B_SHIFT_KEY));

	if (fInWindow) {
		item->SetEnabled(IsCurrentFocusOnTextView()
			|| (FSClipboardHasRefs() && !SelectionIsReadOnly()));

		item->SetTarget(fContainerWindow);
	}

}


void
TShortcuts::UpdateResizeToFitItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(true);
		item->SetTarget(fContainerWindow);
	}
}


void
TShortcuts::UpdateRestoreItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(HasSelection());
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateReverseOrderItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(true);
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateSelectItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(true);
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateSelectAllItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(true);
		item->SetTarget(fContainerWindow);
	}
}


void
TShortcuts::UpdateUnmountItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	item->SetLabel(UnmountLabel());
	item->Message()->what = kUnmountVolume;
	item->SetShortcut(item->Shortcut(), B_COMMAND_KEY);

	if (fInWindow) {
		item->SetEnabled(PoseView()->CanUnmountSelection());
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateUnmountAllItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	item->SetLabel(UnmountAllLabel());
	item->Message()->what = kUnmountAllVolumes;
	item->SetShortcut(item->Shortcut(), B_COMMAND_KEY | B_SHIFT_KEY);

	if (fInWindow) {
		item->SetEnabled(PoseView()->HasUnmountableVolumes());
		item->SetTarget(PoseView());
	}
}


//	#pragma mark - Shortcuts convenience methods


BMenuItem*
TShortcuts::FindItem(BMenu* menu, int32 command1, int32 command2)
{
	// find menu item by either of a a pair of commands
	BMenuItem* item1 = menu->FindItem(command1);
	BMenuItem* item2 = menu->FindItem(command2);
	if (item1 == NULL && item2 == NULL)
		return NULL;

	return item1 != NULL ? item1 : item2;
}


bool
TShortcuts::IsCurrentFocusOnTextView() const
{
	// used to redirect cut/copy/paste and other text-based shortcuts
	if (!fInWindow)
		return false;

	BWindow* window = fContainerWindow;
	return dynamic_cast<BTextView*>(window->CurrentFocus()) != NULL;
}


bool
TShortcuts::IsDesktop() const
{
	return fContainerWindow->TargetModel()->IsDesktop();
}


bool
TShortcuts::IsRoot() const
{
	return fInWindow && fContainerWindow->TargetModel()->IsRoot();
}


bool
TShortcuts::InTrash() const
{
	return fInWindow && fContainerWindow->InTrash();
}


bool
TShortcuts::IsTrash() const
{
	return fInWindow && fContainerWindow->IsTrash();
}


bool
TShortcuts::IsVirtualDirectory() const
{
	return fContainerWindow->TargetModel()->IsVirtualDirectory();
}


bool
TShortcuts::HasSelection() const
{
	return fInWindow && PoseView()->CountSelected() > 0;
}


bool
TShortcuts::ParentIsRoot() const
{
	if (fInWindow)
		return false;

	BEntry entry(fContainerWindow->TargetModel()->EntryRef());
	BDirectory parent;
	return entry.GetParent(&parent) == B_OK
		&& parent.GetEntry(&entry) == B_OK
		&& FSIsRootDir(&entry);
}


bool
TShortcuts::SelectionIsReadOnly() const
{
	return fInWindow && PoseView()->SelectedVolumeIsReadOnly();
}


bool
TShortcuts::TargetIsReadOnly() const
{
	return fInWindow && PoseView()->TargetVolumeIsReadOnly();
}
