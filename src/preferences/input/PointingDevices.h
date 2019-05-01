/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#ifndef POINTING_DEVICES_H
#define POINTING_DEVICES_H


#include <Box.h>
#include <Bitmap.h>
#include <Button.h>
#include <CheckBox.h>
#include <Slider.h>
#include <PopUpMenu.h>
#include <ListView.h>
#include <TabView.h>
#include <View.h>

#include "InputMouse.h"
#include "InputTouchpadPrefView.h"
#include "MouseSettings.h"
#include "MouseView.h"


class BTabView;

class PointingDevices : public BBox {
public:
					PointingDevices();
	virtual     	~PointingDevices();
    void        	SetMouseType(int32 type);
	void 			AttachedToWindow();
	void 			MessageReceived(BMessage* message);
private:
	friend class InputWindow;

	typedef BBox inherited;

	TouchpadPrefView*	fTouchpadPrefView;
	SettingsView*		fSettingsView;
	MouseView*			fMouseView;
	MouseSettings		fSettings;
	BCheckBox*			fAcceptfirstclick;
	BPopUpMenu*			fFocusMenu;
	BSlider*			fCursorSpeedSlider;
	BSlider*			fDoubleClickSpeedSlider;
};

#endif	/* POINTING_DEVICES_H */