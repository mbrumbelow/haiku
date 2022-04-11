/*
 *  Copyright 2010-2020 Haiku, Inc. All rights reserved.
 *  Distributed under the terms of the MIT license.
 *
 *	Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Alexander von Gluck <kallisti5@unixzen.com>
 *		Ryan Leavengood <leavengood@gmail.com>
 *		John Scipione <jscipione@gmail.com>
 */
#ifndef LOOK_AND_FEEL_SETTINGS_VIEW_H
#define LOOK_AND_FEEL_SETTINGS_VIEW_H


#include <DecorInfo.h>
#include <String.h>
#include <View.h>


class BButton;
class BCheckBox;
class BMenuField;
class BPopUpMenu;
class BSlider;
class FakeScrollBar;

class LookAndFeelSettingsView : public BView {
public:
								LookAndFeelSettingsView(const char* name);
	virtual						~LookAndFeelSettingsView();

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

			bool				IsDefaultable();
			void				SetDefaults();

			bool				IsRevertable();
			void				Revert();

private:
			void				_SetDecor(const BString& name);
			void				_SetDecor(BPrivate::DecorInfo* decorInfo);
			void				_BuildDecorMenu();
			const char*			_DecorLabel(const BString& name);

			void				_SetControlLook(const BString& path);
			void				_BuildControlLookMenu();
			const char*			_ControlLookLabel(const char* name);

			bool				_DoubleScrollBarArrows();
			void				_SetDoubleScrollBarArrows(bool doubleArrows);

			uint32				_CursorScale();
			void				_SetCursorScale(uint32 size);
			uint32				_CursorShadow();
			void				_SetCursorShadow(uint32 strength);

private:
			DecorInfoUtility	fDecorUtility;

			BButton*			fDecorInfoButton;
			BMenuField*			fDecorMenuField;
			BPopUpMenu*			fDecorMenu;

			BButton*			fControlLookInfoButton;
			BMenuField*			fControlLookMenuField;
			BPopUpMenu*			fControlLookMenu;

			FakeScrollBar*		fArrowStyleSingle;
			FakeScrollBar*		fArrowStyleDouble;

			BSlider*			fCursorScaleControl;
			BSlider*			fCursorShadowControl;

			BString				fSavedDecor;
			BString				fCurrentDecor;

			BString				fSavedControlLook;
			BString				fCurrentControlLook;

			uint32				fSavedCursorScale;
			uint32				fCurrentCursorScale;
			uint32				fSavedCursorShadow;
			uint32				fCurrentCursorShadow;
			bool				fSavedDoubleArrowsValue : 1;
};


#endif // LOOK_AND_FEEL_SETTINGS_VIEW_H
