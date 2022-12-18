/*
 * Copyright 2022, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef THEME_VIEW_H
#define THEME_VIEW_H


#include <Button.h>
#include <ColorControl.h>
#include <GroupView.h>
#include <ListItem.h>
#include <ListView.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <Messenger.h>
#include <ObjectList.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>
#include <View.h>

#include "Colors.h"


static const uint32 MSG_COLOR_SCHEME_CHANGED 	= 'mccs';
static const uint32 MSG_SET_CURRENT_COLOR	 	= 'sccl';
static const uint32 MSG_UPDATE_COLOR		 	= 'upcl';
static const uint32 MSG_COLOR_ATTRIBUTE_CHOSEN	= 'atch';
static const uint32 MSG_THEME_MODIFIED			= 'tmdf';
static const uint32 MSG_SET_COLOR = 'sclr';

static const char* const kRGBColor = "RGBColor";
static const char* const kName = "name";

class ThemeWindow;
class ColorPreview;
class BMenu;
class BMenuField;
class BPopUpMenu;
class BTextView;

class ThemeView : public BGroupView {
public:
								ThemeView(const char *name,
									const BMessenger &messenger);
	virtual						~ThemeView();

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage *msg);

			void				SetDefaults();
			void				Revert();

private:
			void				_UpdateStyle();
			void				_ChangeColorScheme(color_scheme* scheme);
			void				_SetCurrentColorScheme();
			void				_SetCurrentColor(rgb_color color);

	static	BPopUpMenu*			_MakeColorSchemeMenu(uint32 msg,
									BObjectList<const color_scheme> *items);
	static	void				_MakeColorSchemeMenuItem(uint32 msg,
									const color_scheme *item,
									BPopUpMenu* menu);

private:
			BColorControl*		fPicker;
			BListView*			fAttrList;
			const char*			fName;
			BScrollView*		fScrollView;
			ColorPreview*		fColorPreview;
			BMenuField*			fColorSchemeField;
			BTextView*			fPreview;

			BMessage			fPrevColors;
			BMessage			fDefaultColors;
			BMessage			fCurrentColors;

			BMessenger			fTerminalMessenger;

public:
	static const char* 			kColorTable[];
};

#endif	// THEME_VIEW_H
