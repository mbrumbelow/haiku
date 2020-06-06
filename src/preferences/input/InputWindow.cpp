/*
 * Copyright 2019-2020, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 *		Adrien Destugues <pulkomandy@gmail.com>
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

#include <private/input/InputServerTypes.h>

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
	FindDevice();

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
	switch (message->what) {
		case ITEM_SELECTED:
		{
			int32 index = message->GetInt32("index", 0);
			if (index >= 0)
				fCardView->CardLayout()->SetVisibleItem(index);
			break;
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
			PostMessage(message,
				fCardView->CardLayout()->VisibleItem()->View());
			break;
		}
		case SCROLL_AREA_CHANGED:
		case SCROLL_CONTROL_CHANGED:
		case TAP_CONTROL_CHANGED:
		case DEFAULT_SETTINGS:
		case REVERT_SETTINGS:
		{
			PostMessage(message,
				fCardView->CardLayout()->VisibleItem()->View());
			break;
		}
		case kMsgSliderrepeatrate:
		case kMsgSliderdelayrate:
		{
			PostMessage(message,
				fCardView->CardLayout()->VisibleItem()->View());
			break;
		}

		case IS_NOTIFY_DEVICE:
		{
			bool added = message->FindBool("added");
			BString name = message->FindString("name");

			if (added) {
				BInputDevice* device = find_input_device(name);
				if (device)
					AddDevice(device);
			} else {
				for (int i = 0; i < fDeviceListView->fDeviceList->CountItems();
					i++) {
					BStringItem* item = dynamic_cast<BStringItem*>(
						fDeviceListView->fDeviceList->ItemAt(i));
					if (item->Text() == name) {
						fDeviceListView->fDeviceList->RemoveItem(i);
						BView* settings = fCardView->ChildAt(i);
						fCardView->RemoveChild(settings);
						delete settings;
						break;
					}
				}
			}

			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
InputWindow::Show()
{
	CenterOnScreen();
	BWindow::Show();
	watch_input_devices(this, true);
}


void
InputWindow::Hide()
{
	BWindow::Hide();
	watch_input_devices(this, false);
}


status_t
InputWindow::FindDevice()
{
	BList devList;
	status_t status = get_input_devices(&devList);
	if (status != B_OK)
		return status;

	int32 i = 0;

	fDeviceListView = new DeviceListView(B_TRANSLATE("Device List"));
	fCardView = new BCardView();

	while (true) {
		BInputDevice* dev = (BInputDevice*)devList.ItemAt(i);
		if (dev == NULL) {
			break;
		}
		i++;

		AddDevice(dev);
	}

	return B_OK;
}


void
InputWindow::AddDevice(BInputDevice* dev)
{
	BString name = dev->Name();

	if (dev->Type() == B_POINTING_DEVICE
		&& name.FindFirst("Touchpad") >= 0) {

		TouchpadPrefView* view = new TouchpadPrefView(dev);
		fCardView->AddChild(view);
		DeviceListItemView* touchpad = new DeviceListItemView(
			name, TOUCHPAD_TYPE);
		fDeviceListView->fDeviceList->AddItem(touchpad);
	} else if (dev->Type() == B_POINTING_DEVICE) {
		InputMouse* view = new InputMouse(dev);
		fCardView->AddChild(view);
		DeviceListItemView* mouse = new DeviceListItemView(
			name, MOUSE_TYPE);
		fDeviceListView->fDeviceList->AddItem(mouse);
	} else if (dev->Type() == B_KEYBOARD_DEVICE) {
		InputKeyboard* view = new InputKeyboard(dev);
		fCardView->AddChild(view);
		DeviceListItemView* keyboard = new DeviceListItemView(
			name, KEYBOARD_TYPE);
		fDeviceListView->fDeviceList->AddItem(keyboard);
	} else {
		delete dev;
	}
}
