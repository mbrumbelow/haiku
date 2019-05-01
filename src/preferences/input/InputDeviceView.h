/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#ifndef _INPUT_DEVICE_VIEW_H
#define _INPUT_DEVICE_VIEW_H

#include <ListView.h>
#include <ListItem.h>
#include <StringItem.h>
#include <ScrollBar.h>
#include <String.h>
#include <ScrollView.h>
#include <View.h>


class DeviceName : public BStringItem {
public:
				DeviceName(const char* items);
	virtual 	~DeviceName();
};

class DeviceList : public BListView {
public:
				DeviceList(const char * name,
					list_view_type type = B_SINGLE_SELECTION_LIST,
					uint32  flags = B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE);
	virtual 	~DeviceList();
	// virtual void 	SetSelectionMessage(BMessage* message);
	// virtual	void	MessageReceived(BMessage* message);
};

class DeviceListView: public BView {
public:
				DeviceListView(const char *name);
	virtual		~DeviceListView();
private:
	BScrollView*	fScrollView;
	BListView*		fDeviceList;
};

#endif	// _INPUT_DEVICE_VIEW_H */