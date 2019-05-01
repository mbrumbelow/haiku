/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <LayoutBuilder.h>

#include "InputWindow.h"
#include "InputDeviceView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InputWindow"

InputWindow::InputWindow(BRect _rect)
	:
	BWindow(_rect, B_TRANSLATE_SYSTEM_NAME("Input"), B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS
		| B_AUTO_UPDATE_SIZE_LIMITS | B_QUIT_ON_WINDOW_CLOSE)
{
	BRect rect(0, 0, 300, 200);

	BLayoutBuilder::Group<>(this, B_HORIZONTAL, 10)
		.AddGroup(B_VERTICAL)
		.Add(new DeviceListView(rect, "DeviceList", B_SINGLE_SELECTION_LIST,
			 B_FOLLOW_LEFT_TOP, B_WILL_DRAW))
        .SetInsets(5, 5, 5, 5)
		.End();
}