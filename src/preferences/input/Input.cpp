/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#include "Input.h"
#include "InputWindow.h"
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InputApplication"

const char* kSignature = "application/x-vnd.Haiku-Input";


InputApplication::InputApplication()
	:
	BApplication(kSignature)
{
	BRect rect(0, 0, 600, 500);
	InputWindow *window = new InputWindow(rect);
	window->SetLayout(new BGroupLayout(B_HORIZONTAL));
	window->Show();
}


int
main(int /*argc*/, char ** /*argv*/)
{
	InputApplication app;
	app.Run();

	return 0;
}
