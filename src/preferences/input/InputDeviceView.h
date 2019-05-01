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


class DeviceListView : public BListView {
public:
				DeviceListView(const char * name,
					list_view_type type = B_SINGLE_SELECTION_LIST,
					uint32  flags = B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE);
	virtual 	~DeviceListView();
		// void		_InitWindow();

// private:

	// BListView*		fListView;
};

#endif	// _INPUT_DEVICE_VIEW_H */