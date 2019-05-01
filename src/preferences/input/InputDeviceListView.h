/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
*/


#ifndef INPUT_DEVICE_LIST_VIEW_H_
#define INPUT_DEVICE_LIST_VIEW_H_


#include <ListItem.h>
#include <ListView.h>
#include <View.h>

class InputWindow;

class InputDeviceListView : public BView {
public:
						InputDeviceListView(const char *name);
	virtual				~InputDeviceListView();
private:
    BListView*			fDeviceListView;
	BScrollView*		fScrollView;
};

#endif	// INPUT_DEVICE_LIST_VIEW_H_