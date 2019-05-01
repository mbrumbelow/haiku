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

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InputWindow"

InputWindow::InputWindow(BRect _rect)
	:
	BWindow(_rect, B_TRANSLATE_SYSTEM_NAME("Input"), B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS
		| B_AUTO_UPDATE_SIZE_LIMITS)
{
	BLayoutBuilder::Group<>(this, B_VERTICAL, 10)
		.AddGroup(B_HORIZONTAL)
        .SetInsets(5, 5, 5, 5)
		.End();
}


bool
InputWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_ON_WINDOW_CLOSE);

	return true;
}