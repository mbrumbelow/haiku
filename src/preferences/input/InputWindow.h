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

class InputMouseView;
class DeviceListView;
class DeviceName;
// class BCardLayout;


class InputWindow : public BWindow
{
public:
							InputWindow(BRect rect);
	virtual void 			MessageReceived(BMessage* message);
private:
	InputMouseView*			fInputMouseView;
	DeviceListView*				fDeviceListView;
	BSeparatorView*			fTitleView;
	BButton 				*fDefaults;
	BButton 				*fRevert;
	BBox					*fSettingsBox;
	// BCardLayout*			fContentLayout;
};

#endif /* INPUT_WINDOW_H */
