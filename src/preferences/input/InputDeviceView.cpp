/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
*/


#include "InputDeviceView.h"


#include <Catalog.h>
#include <DateFormat.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <Locale.h>
#include <ScrollView.h>
#include <String.h>
#include <StringItem.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DeviceList"


DeviceList::DeviceList(const char* name,
					list_view_type type, uint32 flags)
	:
	 BListView(name, type, flags)
{
}

DeviceList::~DeviceList()
{
}

DeviceName::DeviceName(const char* item, int d)
	:
	BStringItem(item)
{
	this->fDevice = d;
}

DeviceName::~DeviceName()
{
}

DeviceListView::DeviceListView(const char* name)
	:
	BView(name, B_WILL_DRAW)
{
	fDeviceList = new DeviceList("Device Names");

	fScrollView = new BScrollView("ScrollView",fDeviceList,
					0 , false, B_FANCY_BORDER);

	SetExplicitMinSize(BSize(160, 20));
	SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 20));

	if (fMouseSettings.IsMouseConnected() == true) {
	fDeviceList->AddItem(new DeviceName("Mouse", 101));
	}

	if (fTouchpadPref.IsTouchpadConnected() == true) {
	fDeviceList->AddItem(new DeviceName("Touchpad", 102));
	}
	
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(fScrollView)
		.End();
	fDeviceList->SetSelectionMessage(new BMessage(ITEM_SELECTED));
}

DeviceListView::~DeviceListView()
{
}

void
DeviceListView::AttachedToWindow()
{
	fDeviceList->SetTarget(this);

	fDeviceList->Select(0);
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
}