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
#include <StringItem.h>
#include <ScrollView.h>

#include "InputDeviceView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DeviceList"


DeviceListView::DeviceListView(const char* name,
					list_view_type type, uint32 flags)
	:
	 BListView(name, type, flags)
{
}

DeviceListView::~DeviceListView()
{
}

DeviceList::DeviceList(const char* text)
	:
	BStringItem(text)
{
}

DeviceList::~DeviceList()
{
}
