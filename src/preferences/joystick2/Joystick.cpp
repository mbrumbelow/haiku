/*
 * Copyright 2020, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#include "Joystick.h"

#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>

#include "JoystickWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "JoystickApplication"

const char* kSignature = "application/x-vnd.Haiku-Joystick";


JoystickApplication::JoystickApplication()
	:
	BApplication(kSignature)
{
	BRect rect(0, 0, 600, 500);
	JoystickWindow* window = new JoystickWindow(rect);
	window->Show();
}


int
main(int /*argc*/, char** /*argv*/)
{
	JoystickApplication app;
	app.Run();

	return 0;
}
