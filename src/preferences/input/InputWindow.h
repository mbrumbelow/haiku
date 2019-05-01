/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */

#ifndef INPUT_WINDOW_H
#define INPUT_WINDOW_H

#include <Window.h>
#include <View.h>
#include <ListItem.h>
#include <ListView.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <Box.h>
#include <Message.h>

// #include "MouseSettings.h"
// #include "InputMouse.h"

class BSplitView;

class InputMouseTouchpadView;
class DeviceListView;
class DeviceName;
// class InputKeyboardView;
// class BCardLayout;


class InputWindow : public BWindow
{
public:
							InputWindow(BRect rect);
	virtual void 			MessageReceived(BMessage* message);

private:
	BSplitView*				fMainSplitView;
	InputMouseTouchpadView*	fInputMouseTouchpadView;
	DeviceListView*			fDeviceListView;
	// MouseSettings			fSettings;
	// MousePref*				fMousePref;
	// InputKeyboardView*		fInputKeyboardView;
	BButton 				*fDefaults;
	BButton 				*fRevert;
	BBox					*fSettingsBox;
	// BCardLayout*			fContentLayout;
};

#endif /* INPUT_WINDOW_H */
