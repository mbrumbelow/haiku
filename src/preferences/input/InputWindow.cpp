/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <SeparatorView.h>


#include "InputWindow.h"
#include "InputDeviceView.h"
#include "InputDeviceListView.h"
#include "Input_Mouse_Touchpad.h"
#include "InputMouseView.h"
#include "InputMouseSettings.h"


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
	fDefaults = new BButton("defaults", B_TRANSLATE("Defaults"),
		new BMessage(kMsgDefaults));

	fReverts = new BButton("reverts", B_TRANSLATE("Reverts"),
		new BMessage(kMsgRevert));

	fInputMouseView = new InputMouseView();
	fDeviceListView = new InputDeviceListView("List");

	// fMouseView = new MouseView(fSettings);

	BLayoutBuilder::Group<>(this, B_HORIZONTAL, 10)
		.AddGroup(B_HORIZONTAL,0)
		.Add(new DeviceListView("Device List", B_SINGLE_SELECTION_LIST,
			B_WILL_DRAW))
		.Add(fDeviceListView)
		.End()

		.AddGroup(B_VERTICAL,0)
		.Add(fInputMouseView)
		.Add(new BSeparatorView(B_HORIZONTAL))

		.AddGroup(B_VERTICAL,0)
			.AddGroup(B_HORIZONTAL,0)
			.SetInsets(B_USE_WINDOW_SPACING, 0, B_USE_WINDOW_SPACING,
				B_USE_WINDOW_SPACING)
				.Add(fDefaults)
				.Add(fReverts)
				.End()
			.End();
}