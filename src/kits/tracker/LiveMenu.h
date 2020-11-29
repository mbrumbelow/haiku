/*
 * Copyright 2020-2023 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */
#ifndef _LIVE_MENU_H
#define _LIVE_MENU_H


#include <Menu.h>
#include <PopUpMenu.h>


namespace BPrivate {

class BContainerWindow;

// mixin class for sharing virtual methods
struct TLiveMixin {
	virtual	void			UpdateFileMenu(BMenu* menu);
	virtual	void			UpdateWindowMenu(BMenu* menu);
};


class TLiveMenu : public BMenu {
public:
							TLiveMenu(const char* label);
	virtual 				~TLiveMenu();

	virtual	void			MessageReceived(BMessage* message);

protected:
	virtual	void			Update();
};


class TLivePopUpMenu : public BPopUpMenu {
public:
							TLivePopUpMenu(const char* label,
								bool radioMode = true,
								bool labelFromMarked = true,
								menu_layout layout = B_ITEMS_IN_COLUMN);
	virtual					~TLivePopUpMenu();

	virtual	void			MessageReceived(BMessage* message);

protected:
	virtual	void			Update();
};


class TLiveArrangeByMenu: public TLiveMenu, public TLiveMixin {
public:
							TLiveArrangeByMenu(const char* label);
	virtual					~TLiveArrangeByMenu();

protected:
	virtual	void			Update();
};


class TLiveFileMenu : public TLiveMenu, public TLiveMixin {
public:
							TLiveFileMenu(const char* label);
	virtual					~TLiveFileMenu();

protected:
	virtual	void			Update();
};

class TLivePosePopUpMenu : public TLivePopUpMenu, public TLiveMixin {
public:
							TLivePosePopUpMenu(const char* label,
								bool radioMode = true,
								bool labelFromMarked = true,
								menu_layout layout = B_ITEMS_IN_COLUMN);
	virtual					~TLivePosePopUpMenu();

protected:
	virtual	void			Update();
};


class TLiveWindowMenu : public TLiveMenu, public TLiveMixin {
public:
							TLiveWindowMenu(const char* label);
	virtual					~TLiveWindowMenu();

protected:
	virtual	void			Update();
};

class TLiveWindowPopUpMenu : public TLivePopUpMenu, public TLiveMixin {
public:
							TLiveWindowPopUpMenu(const char* label,
								bool radioMode = true,
								bool labelFromMarked = true,
								menu_layout layout = B_ITEMS_IN_COLUMN);
	virtual					~TLiveWindowPopUpMenu();

protected:
	virtual	void			Update();
};

} // namespace BPrivate

using namespace BPrivate;


#endif	// _LIVE_MENU_H
