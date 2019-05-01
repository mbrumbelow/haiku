/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
*/


#ifndef INPUT_MOUSE_H
#define INPUT_MOUSE_H


#include <Box.h>
#include <Bitmap.h>
#include <Button.h>
#include <CheckBox.h>
#include <Slider.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <ListView.h>
#include <TabView.h>
#include <View.h>

#include "SettingsView.h"
#include "MouseSettings.h"
#include "MouseView.h"
#include "InputWindow.h"

#define MOUSE_SETTINGS 'Mss'


class BTabView;

class InputMouse : public BView {
public:
					InputMouse(const char* name);
	virtual     	~InputMouse();
    void        	SetMouseType(int32 type);
	void 			MessageReceived(BMessage* message);
private:
	friend class InputWindow;

	typedef BBox inherited;

	SettingsView*		fSettingsView;
	MouseView*			fMouseView;
	BButton				*fDefaultsButton;
	BButton				*fRevertButton;
	MouseSettings		fSettings;

	mouse_settings		fMouseSettings, fOriginalSettings;
};

#endif	/* INPUT_MOUSE_H */