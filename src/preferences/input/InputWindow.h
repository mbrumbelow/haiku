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
#include <Box.h>

class InputMouseView;
class DeviceListView;
// class MouseView;
// class InputMouseSettings;
// class Settings;

class InputWindow : public BWindow
{
public:
	InputWindow(BRect rect);

private:
	// InputMouseSettings		fMouseSettings;
	// Settings				*fSettings;
	InputMouseView*			fInputMouseView;
	DeviceListView*			fDeviceListView;
	// MouseView*				fMouseView;
	BScrollView 			*fScrollView;
	BButton 				*fDefaults;
	BButton 				*fRevert;
	BBox				*fSettingsBox;
};

#endif /* INPUT_WINDOW_H */
