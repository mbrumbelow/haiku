/*
 * Copyright 2020, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */
#ifndef _LIVE_UPDATING_MENU_H
#define _LIVE_UPDATING_MENU_H


#include <Menu.h>
#include <PopUpMenu.h>


namespace BPrivate {

class TLiveUpdatingMenu : public BMenu {
public:
							TLiveUpdatingMenu(const char* label);
	virtual 				~TLiveUpdatingMenu();

	virtual	void			MessageReceived(BMessage* message);
	virtual	void			UpdateMenuItemsForModifiersChanged();
};

class TLiveUpdatingPopUpMenu : public BPopUpMenu {
public:
							TLiveUpdatingPopUpMenu(const char* label,
								bool radioMode = true,
								bool labelFromMarked = true,
								menu_layout layout = B_ITEMS_IN_COLUMN);
	virtual					~TLiveUpdatingPopUpMenu();

	virtual	void			MessageReceived(BMessage* message);
	virtual	void			UpdateMenuItemsForModifiersChanged();
};


class TLiveUpdatingArrangeByMenu: public TLiveUpdatingMenu {
public:
							TLiveUpdatingArrangeByMenu(const char* label);
	virtual					~TLiveUpdatingArrangeByMenu();

	virtual	void			UpdateMenuItemsForModifiersChanged();

	virtual	void			UpdateCleanUpMenuItem(BMenuItem* item);
};

// mixin class for file menu and file context menu
struct TFileMixin {
	virtual	void			UpdateCreateLinkMenuItem(BMenuItem* item);
	virtual	void			UpdateCutMenuItem(BMenuItem* item);
	virtual	void			UpdateCopyMenuItem(BMenuItem* item);
	virtual	void			UpdatePasteMenuItem(BMenuItem* item);
	virtual	void 			UpdateIdentifyMenuItem(BMenuItem* item);

protected:
	virtual	void			UpdateCreateLinkMenuItemsInMenu(BMenu* menu);
};

class TLiveUpdatingFileMenu : public TLiveUpdatingMenu,
	public TFileMixin {
public:
							TLiveUpdatingFileMenu(const char* label);
	virtual					~TLiveUpdatingFileMenu();

	virtual	void			UpdateMenuItemsForModifiersChanged();
};

class TLiveUpdatingFilePopUpMenu : public TLiveUpdatingPopUpMenu,
	public TFileMixin {
public:
							TLiveUpdatingFilePopUpMenu(const char* label,
								bool radioMode = true,
								bool labelFromMarked = true,
								menu_layout layout = B_ITEMS_IN_COLUMN);
	virtual					~TLiveUpdatingFilePopUpMenu();

	virtual	void			UpdateMenuItemsForModifiersChanged();
};

class TLiveUpdatingWindowMenu: public TLiveUpdatingMenu {
public:
							TLiveUpdatingWindowMenu(const char* label);
	virtual					~TLiveUpdatingWindowMenu();

	virtual	void			UpdateMenuItemsForModifiersChanged();

	virtual	void			UpdateCloseMenuItem(BMenuItem* item);
};

} // namespace BPrivate

using namespace BPrivate;


#endif	// _LIVE_UPDATING_MENU_H
