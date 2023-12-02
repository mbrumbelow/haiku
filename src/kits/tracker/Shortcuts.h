/*
 * Copyright 2020-2023 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */
#ifndef _SHORTCUTS_H
#define _SHORTCUTS_H


#include <SupportDefs.h>


class BMenu;
class BMenuItem;


namespace BPrivate {

class Shortcuts {
public:
								Shortcuts();

			BMenuItem*			AddPrinterItem();
			const char*			AddPrinterLabel();

			BMenuItem*			CleanupItem();
			const char*			CleanupLabel();
			int32				CleanupCommand();

			BMenuItem*			CloseItem();
			const char*			CloseLabel();
			int32				CloseCommand();

			BMenuItem*			CloseAllInWorkspaceItem();
			const char*			CloseAllInWorkspaceLabel();

			BMenuItem*			CopyItem();
			const char*			CopyLabel();
			int32				CopyCommand();

			BMenuItem*			CopyToItem(BMenu* menu);
			const char*			CopyToLabel();

			BMenuItem*			CreateLinkItem(BMenu* menu);
			const char*			CreateLinkLabel();
			int32				CreateLinkCommand();

			BMenuItem*			CreateLinkHereItem();
			const char*			CreateLinkHereLabel();
			int32				CreateLinkHereCommand();

			BMenuItem*			CutItem();
			const char*			CutLabel();
			int32				CutCommand();

			BMenuItem*			DeleteItem();
			const char*			DeleteLabel();

			BMenuItem*			DuplicateItem();
			const char*			DuplicateLabel();

			BMenuItem*			EditNameItem();
			const char*			EditNameLabel();

			BMenuItem*			EditQueryItem();
			const char*			EditQueryLabel();

			BMenuItem*			EmptyTrashItem();
			const char*			EmptyTrashLabel();

			BMenuItem*			FindItem();
			const char*			FindLabel();

			BMenuItem*			GetInfoItem();
			const char*			GetInfoLabel();

			BMenuItem*			IdentifyItem();
			const char*			IdentifyLabel();

			BMenuItem*			InvertSelectionItem();
			const char*			InvertSelectionLabel();

			BMenuItem*			MakeActivePrinterItem();
			const char*			MakeActivePrinterLabel();

			const char*			MountLabel();

			BMenuItem*			MoveToItem(BMenu* menu);
			const char*			MoveToLabel();

			BMenuItem*			MoveToTrashItem();
			const char*			MoveToTrashLabel();
			int32				MoveToTrashCommand();

			BMenuItem*			NewFolderItem();
			const char*			NewFolderLabel();

			BMenuItem*			OpenItem();
			const char*			OpenLabel();

			BMenuItem*			OpenParentItem();
			const char*			OpenParentLabel();

			BMenuItem*			OpenWithItem();
			BMenuItem*			OpenWithItem(BMenu* menu);
			const char*			OpenWithhLabel();

			BMenuItem*			PasteItem();
			const char*			PasteLabel();
			int32				PasteCommand();

			BMenuItem*			RestoreItem();
			const char*			RestoreLabel();

			BMenuItem*			ReverseOrderItem();
			const char*			ReverseOrderLabel();

			BMenuItem*			ResizeToFitItem();
			const char*			ResizeToFitLabel();

			BMenuItem*			SelectItem();
			const char*			SelectLabel();

			BMenuItem*			SelectAllItem();
			const char*			SelectAllLabel();

			BMenuItem*			UnmountItem();
			const char*			UnmountLabel();

			BMenuItem*			UnmountAllItem();
			const char*			UnmountAllLabel();

			void				UpdateCleanupItem(BMenuItem* item);
			void				UpdateCloseItem(BMenuItem* item);
			void				UpdateCreateLinkItem(BMenuItem* item);
			void				UpdateCreateLinkHereItem(BMenuItem* item);
			void				UpdateCreateLinkInMenu(BMenu* menu);
			void				UpdateCutItem(BMenuItem* item);
			void				UpdateCopyItem(BMenuItem* item);
			void				UpdateDeleteItem(BMenuItem* item);
			void				UpdateIdentifyItem(BMenuItem* item);
			void				UpdateMoveToTrashItem(BMenuItem* item);
			void				UpdatePasteItem(BMenuItem* item);
};

}

using namespace BPrivate;


#endif	// _SHORTCUTS_H
