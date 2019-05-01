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

class InputMouseView;
class InputDeviceListView;
class DeviceList;
class MouseView;

class InputWindow : public BWindow
{
public:
	InputWindow(BRect rect);

private:
	InputMouseView*			fInputMouseView;
	InputDeviceListView*	fDeviceListView;
	// DeviceList			*fDeviceList;
	MouseView*				fMouseView;
	BScrollView 			*fScrollView;
	BButton 				*fDefaults;
	BButton 				*fReverts;
};

#endif /* INPUT_WINDOW_H */
