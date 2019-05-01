/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#ifndef INPUT_MOUSE_TOUCHPAD_H
#define INPUT_MOUSE_TOUCHPAD_H


#include <Box.h>
#include <Bitmap.h>
#include <Button.h>
#include <CheckBox.h>
#include <Slider.h>
#include <PopUpMenu.h>
#include <TabView.h>

#include "InputMouse.h"


class InputMouseView : public BBox {
public:
		        InputMouseView();
	virtual     ~InputMouseView();
    void         SetMouseType(int32 type);
private:
	friend class InputWindow;

	typedef BBox inherited;

	MousePref*		fMousePref;
	BButton*		fDefaults;
	BButton*		fReverts;
	BCheckBox*		fAcceptfirstclick;
	BPopUpMenu*		fFocusMenu;
	BSlider*		fCursorSpeedSlider;
	BSlider*		fDoubleClickSpeedSlider;
	BSlider*		fSliderDemo;
	BSlider*		fSliderDemo1;
};

#endif	/* INPUT_MOUSE_TOUCHPAD_H */