/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
*/


#include <ListView.h>
#include <Catalog.h>
#include <DateFormat.h>
#include <Locale.h>
#include <String.h>

#include "InputDeviceView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DeviceList"


DeviceListView::DeviceListView(BRect frame, const char* name,
					list_view_type type, uint32 resizingMode, uint32 flags)
	:
	 BListView(frame, name, type, resizingMode, flags)
{
}

DeviceListView::~DeviceListView()
{
}

/*void
DeviceListView::_InitWindow()
{
	fListView = new BListView("device_list_view");
	fListView->SetExplicitMinSize(BSize(140, 100));
}*/
