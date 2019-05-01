/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
*/


#ifndef SETTINGS_VIEW_H
#define SETTINGS_VIEW_H


#include <Box.h>
#include <Bitmap.h>
#include <Button.h>
#include <PopUpMenu.h>


class MouseSettings;
class MouseView;


class SettingsView : public BControl {
public:
		            SettingsView(MouseSettings &settings);
	virtual         ~SettingsView();
	virtual void 	AttachedToWindow();

	void 			SetMouseType(int32 type);
	void 			MouseMapUpdated();
	void 			UpdateFromSettings();
private:
	friend class InputWindow;

	typedef BBox inherited;

	const MouseSettings &fSettings;

    BPopUpMenu*     fTypeMenu;
	MouseView*		fMouseView;
};

#endif	/* SETTINGS_VIEW_H */
