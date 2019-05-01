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
#include <Catalog.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <SeparatorView.h>
#include <SplitView.h>
#include <TabView.h>
#include <stdio.h>


#include "InputWindow.h"
#include "InputDeviceView.h"
#include "Input_Mouse_Touchpad.h"
#include "MouseSettings.h"
#include "InputMouse.h"
// #include "InputMouse.h"
// #include "InputKeyboard.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InputWindow"


const uint32 kMsgDefaults	= 'BTde';
const uint32 kMsgRevert		= 'BTre';


InputWindow::InputWindow(BRect _rect)
	:
	BWindow(_rect, B_TRANSLATE_SYSTEM_NAME("Input"), B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS
		| B_AUTO_UPDATE_SIZE_LIMITS | B_QUIT_ON_WINDOW_CLOSE)
{
	float padding = be_control_look->DefaultItemSpacing();

	fDefaults = new BButton("defaults", B_TRANSLATE("Defaults"),
		new BMessage(kMsgDefaults));

	fRevert = new BButton("revert", B_TRANSLATE("Revert"),
		new BMessage(kMsgRevert));

	fInputMouseTouchpadView = new InputMouseTouchpadView();
	fDeviceListView = new DeviceListView(B_TRANSLATE("Device List"));
	// fInputKeyboardView = new InputKeyboardView();

	fMainSplitView = new BSplitView(B_HORIZONTAL, floorf(padding / 2));
	// fMousePref = new MousePref(fSettings);

	BLayoutBuilder::Group<>(this, B_HORIZONTAL, 10)
		.Add(fDeviceListView)
		.AddGroup(B_VERTICAL,0)
			.Add(fInputMouseTouchpadView)
			// .Add(fMousePref)
			// .Add(fInputKeyboardView)
			.Add(fMainSplitView)
			.Add(new BSeparatorView(B_HORIZONTAL))

		.AddGroup(B_HORIZONTAL,0)
			.SetInsets(B_USE_WINDOW_SPACING, 0, B_USE_WINDOW_SPACING,
				B_USE_WINDOW_SPACING)
				.Add(fDefaults)
				.Add(fRevert)
				.End();
}


void
InputWindow::MessageReceived(BMessage* message)
{
	message->PrintToStream();
	int32 name = message->GetInt32("index", 1);
		switch (message->what) {
			case ITEM_SELECTED:
				switch (name){
					case 0:
						printf("mouse case 0\n");
						fInputMouseTouchpadView->HidePref();
						break;
					case 1:
						printf("keyboard case 1\n");
						fInputMouseTouchpadView->ShowPref();
						break;
					default:
						break;
				}
				// if (name == 0)
				// {
				// 	fInputMouseTouchpadView->ShowPref();
				// }
				// else if (name == 1)
				// {
				// 	fInputMouseTouchpadView->HidePref();
				// }
				// break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}