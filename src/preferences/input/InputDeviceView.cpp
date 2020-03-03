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
#include <Input.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <Locale.h>
#include <ScrollView.h>
#include <String.h>
#include <StringItem.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DeviceList"


DeviceListView::DeviceListView(const char* name)
	:
	BView(name, B_WILL_DRAW)
{
	fDeviceList = new BListView("Device Names");

	fScrollView = new BScrollView("ScrollView",fDeviceList,
					0 , false, B_FANCY_BORDER);

	SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
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

void
DeviceListView::SetExplicitSize()
{
	float minWidth = 0;
	for (int32 i = 0; i < fDeviceList->CountItems(); i++) {
		BStringItem* item = (BStringItem*)fDeviceList->ItemAt(i);
		float width = fDeviceList->StringWidth(item->Text());
		if (width > minWidth){
			minWidth = width;
		}
	}
	SetExplicitMinSize(BSize(minWidth + 40 , B_SIZE_UNSET));
}
