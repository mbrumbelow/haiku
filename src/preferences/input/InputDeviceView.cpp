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
#include <LayoutBuilder.h>


#include "InputDeviceView.h"
#include "InputWindow.h"
#include "InputMouseConstants.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DeviceList"
#define ITEM_SELECTED 'I1s'
// #define ITEM_2_SELECTED 'I2s'


DeviceList::DeviceList(const char* name,
					list_view_type type, uint32 flags)
	:
	 BListView(name, type, flags)
{
}
		// this->SetSelectionMessage();

DeviceList::~DeviceList()
{
}

/*void
DeviceListView::MessageReceived(BMessage* message)
{
	message->PrintToStream();
	switch(message->what)
	{
		case ITEM_SELECTED:
		{
			fDeviceListView->SetSelectionMessage(message);
		}
	}
}*/

DeviceName::DeviceName(const char* items)
	:
	BStringItem(items)
{
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

	fDeviceList->AddItem(new DeviceName("Mouse/Touchpad"));
	fDeviceList->AddItem(new DeviceName("Keyboard/Keymap"));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(fScrollView)
		// .Add(fDeviceList)
		.End();
}

DeviceListView::~DeviceListView()
{
}
