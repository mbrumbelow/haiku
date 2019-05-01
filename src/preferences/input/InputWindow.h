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

#include "MouseSettings.h"
#include "InputMouse.h"


class BSplitView;

class PointingDevices;
class DeviceListView;
class SettingsView;
class DeviceName;


class InputWindow : public BWindow
{
public:
							InputWindow(BRect rect);

private:
	PointingDevices*		fPointingDevices;
	DeviceListView*			fDeviceListView;
	BButton 				*fDefaults;
	BButton 				*fRevert;
	MouseSettings			fSettings;
	SettingsView			*fSettingsView;
};

#endif /* INPUT_WINDOW_H */
