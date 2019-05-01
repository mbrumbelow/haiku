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
// #include "MouseSettings.h"



class InputMouseTouchpadView : public BBox {
public:
		        InputMouseTouchpadView();
	virtual     ~InputMouseTouchpadView();
    void        SetMouseType(int32 type);
	void 		ShowPref();
	void		HidePref();
private:
	friend class InputWindow;

	typedef BBox inherited;

	// MouseSettings	fSettings;
	// MousePref*		fMousePref;
	BCheckBox*		fAcceptfirstclick;
	BPopUpMenu*		fFocusMenu;
	BSlider*		fCursorSpeedSlider;
	BSlider*		fDoubleClickSpeedSlider;
};

#endif	/* INPUT_MOUSE_TOUCHPAD_H */