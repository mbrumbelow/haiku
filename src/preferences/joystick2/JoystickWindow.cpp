/*
 * Copyright 2020, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */

#include "JoystickWindow.h"

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

#include "JoystickView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "JoystickWindow"


JoystickWindow::JoystickWindow(BRect rect)
	:
	BWindow(rect, B_TRANSLATE_SYSTEM_NAME("Joystick"), B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS
		| B_AUTO_UPDATE_SIZE_LIMITS | B_QUIT_ON_WINDOW_CLOSE)
{
	fJoystickView = new JoystickView();

	BLayoutBuilder::Group<>(this, B_HORIZONTAL, 10)
		.SetInsets(B_USE_WINDOW_SPACING)
		.Add(fJoystickView)
		.End();
}

void
JoystickWindow::Show()
{
	CenterOnScreen();
	BWindow::Show();
}


void
JoystickWindow::Hide()
{
	BWindow::Hide();
}

