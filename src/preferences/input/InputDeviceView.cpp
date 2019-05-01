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
#include "InputWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DeviceList"


DeviceListView::DeviceListView(const char* name,
					list_view_type type, uint32 flags)
	:
	 BListView(name, type, flags)
{
	const char* item1 = B_TRANSLATE_NOCOLLECT("Mouse/Touchpad");
	this->AddItem(new DeviceName(item1));

	const char* item2 = B_TRANSLATE_NOCOLLECT("Keyboard/Keymap");
	this->AddItem(new DeviceName(item2));
}

DeviceListView::~DeviceListView()
{
}

DeviceName::DeviceName(const char* items)
	:
	BStringItem(items)
{
}

DeviceName::~DeviceName()
{
}