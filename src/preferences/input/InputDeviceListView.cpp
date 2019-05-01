/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
*/



#include <stdio.h>

#include <Alert.h>
#include <Catalog.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Messenger.h>
#include <Path.h>
#include <SpaceLayoutItem.h>

#include "InputWindow.h"
#include "InputDeviceView.h"
#include "InputDeviceListView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Input Device List View"


InputDeviceListView::InputDeviceListView(const char* name)
	:
	BView(name, B_WILL_DRAW)
{
	fDeviceListView = new DeviceListView("DeviceList");

	fScrollView = new BScrollView("ScrollView", fDeviceListView,
					0, false, true);
	fScrollView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	const char* text = B_TRANSLATE_NOCOLLECT("Mouse");
	fDeviceListView->AddItem(new DeviceList(text));
}

InputDeviceListView::~InputDeviceListView()
{
}