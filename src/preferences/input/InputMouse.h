/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#ifndef INPUT_MOUSE_H
#define INPUT_MOUSE_H


#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <ListView.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <Slider.h>
#include <TabView.h>
#include <View.h>

#include "InputDeviceView.h"
#include "MouseView.h"
#include "SettingsView.h"

#define MOUSE_SETTINGS 'Mss'

class DeviceListView;
class TMouseSettings;


class InputMouse : public BView {
public:
					InputMouse(BInputDevice* dev, TMouseSettings* settings);
	virtual			~InputMouse();
	void			SetMouseType(int32 type);
	void			MessageReceived(BMessage* message);
private:

	typedef BBox inherited;

	SettingsView*		fSettingsView;
	MouseView*			fMouseView;
	BButton*			fDefaultsButton;
	BButton*			fRevertButton;
	TMouseSettings*		fSettings;
};

#endif	/* INPUT_MOUSE_H */
