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
#include "Input_Mouse_Touchpad.h"

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

	fRevert = new BButton("revert", B_TRANSLATE("Revert"),
		new BMessage(kMsgRevert));

	fInputMouseView = new InputMouseView();
	fDeviceListView = new DeviceListView(B_TRANSLATE("Device List"));

	/*fContentLayout = new BCardLayout();
	new BView("content view", 0, fContentLayout);
	fContentLayout->Owner()->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	fContentLayout->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fInputMouseView = new InputMouseView();
	fContentLayout->AddView(fInputMouseView);*/

	BLayoutBuilder::Group<>(this, B_HORIZONTAL, 10)
		.Add(fDeviceListView)
		.AddGroup(B_VERTICAL,0)
			.Add(fInputMouseView)
			.Add(new BSeparatorView(B_HORIZONTAL))

		.AddGroup(B_HORIZONTAL,0)
			.SetInsets(B_USE_WINDOW_SPACING, 0, B_USE_WINDOW_SPACING,
				B_USE_WINDOW_SPACING)
				.Add(fDefaults)
				.Add(fRevert)
				.End()
			.End();
}


void
InputWindow::MessageReceived(BMessage* message)
{
	//  message->PrintToStream();
	// switch(message->what)
	// {
		// this->fDeviceListView-;
	// }
}