/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#include <Alert.h>
#include <Alignment.h>
#include <Application.h>
#include <Button.h>
#include <CardLayout.h>
#include <CardView.h>
#include <Catalog.h>
#include <Control.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <SplitView.h>
#include <Screen.h>
#include <stdio.h>
#include <TabView.h>


#include "InputConstants.h"
#include "InputDeviceView.h"
#include "InputMouse.h"
#include "InputTouchpadPref.h"
#include "InputWindow.h"
#include "MouseSettings.h"
#include "SettingsView.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InputWindow"


InputWindow::InputWindow(BRect rect)
	:
	BWindow(rect, B_TRANSLATE_SYSTEM_NAME("Input"), B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS
		| B_AUTO_UPDATE_SIZE_LIMITS | B_QUIT_ON_WINDOW_CLOSE)
{
	fInputMouse = new InputMouse();
	fTouchpadPrefView = new TouchpadPrefView(B_TRANSLATE("Touchpad"));
	fDeviceListView = new DeviceListView(B_TRANSLATE("Device List"));

	fCardView = new BCardView();

	ConnectToDevice();
	if (fMouse)
	{
		fCardView->AddChild(fInputMouse);
	}
	if (fTouchpad)
	{
		fCardView->AddChild(fTouchpadPrefView);
	}

	BLayoutBuilder::Group<>(this, B_HORIZONTAL, 10)
		.SetInsets(B_USE_WINDOW_SPACING)
		.Add(fDeviceListView)
		.AddGroup(B_VERTICAL, 0)
			.Add(fCardView)
		.End();
}

void
InputWindow::MessageReceived(BMessage* message)
{
	int32 name = message->GetInt32("index", 0);

	switch (message->what) {

		case ITEM_SELECTED:
		{
			fCardView->CardLayout()->SetVisibleItem(name);
		}
		case kMsgMouseType:
		case kMsgMouseMap:
		case kMsgMouseFocusMode:
		case kMsgFollowsMouseMode:
		case kMsgAcceptFirstClick:
		case kMsgDoubleClickSpeed:
		case kMsgMouseSpeed:
		case kMsgAccelerationFactor:
		case kMsgDefaults:
		case kMsgRevert:
		{
			PostMessage(message, fInputMouse);
			break;
		}
		case SCROLL_AREA_CHANGED:
		case SCROLL_CONTROL_CHANGED:
		case TAP_CONTROL_CHANGED:
		case DEFAULT_SETTINGS:
		case REVERT_SETTINGS:
		{
			PostMessage(message, fTouchpadPrefView);
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}

status_t
InputWindow::ConnectToDevice()
{
	BList devList;
	status_t status = get_input_devices(&devList);
	if (status != B_OK)
		return status;

	int32 i = 0;
	while (true) {
		BInputDevice* dev = (BInputDevice*)devList.ItemAt(i);
		if (dev == NULL)
			break;
		i++;

		dev->Name();

		BString name = dev->Name();

		if (name.FindFirst("Mouse") >= 0
			&& dev->Type() == B_POINTING_DEVICE
			&& !fConnected) {
			fConnected = true;
			fMouse = dev;
			fprintf(stderr, (name));
			fDeviceListView->fDeviceList->AddItem(new BStringItem (name));

		if (name.FindFirst("Touchpad") >= 0
			&& dev->Type() == B_POINTING_DEVICE
			&& !fConnected) {
			fConnected = true;
			fTouchpad = dev;
			fprintf(stderr, (name));
			fDeviceListView->fDeviceList->AddItem(new BStringItem (name));
			}
		} else {
			delete dev;
		}
	}
	if (fConnected)
		return B_OK;
	return B_ENTRY_NOT_FOUND;
}
