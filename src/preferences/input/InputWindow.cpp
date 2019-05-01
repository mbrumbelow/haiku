/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#include <Alert.h>
#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <StringView.h>

#include "InputWindow.h"
#include "InputDeviceView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InputWindow"


static const uint32 kMsgDefaults = 'dflt';


InputWindow::InputWindow(BRect _rect)
	:
	BWindow(_rect, B_TRANSLATE_SYSTEM_NAME("Input"), B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS
		| B_AUTO_UPDATE_SIZE_LIMITS | B_QUIT_ON_WINDOW_CLOSE)
{
	fStringView = new BStringView("message",
	B_TRANSLATE("More things will be added here."));

	BBox* box = new BBox("box");

	box->AddChild(BLayoutBuilder::Group<>(B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(fStringView)
		.View());

	fDefaults = new BButton("defaults", B_TRANSLATE("Defaults"),
		new BMessage(kMsgDefaults));

	BLayoutBuilder::Group<>(this, B_HORIZONTAL, 10)
		.AddGroup(B_VERTICAL)
		.Add(new DeviceListView("DeviceList", B_SINGLE_SELECTION_LIST,
			B_WILL_DRAW))
		.Add(box)
		.Add(fDefaults)
        .SetInsets(5, 5, 5, 5)
		.End();
}