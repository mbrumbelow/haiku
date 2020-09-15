/*
 * Copyright 2002-2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */


#include "GuideWindow.h"

#include <Application.h>
#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GuideWindow"


GuideWindow::GuideWindow()
	:
	BWindow(BRect(50, 50, 600, 500), B_TRANSLATE_SYSTEM_NAME("DriveSetupGuided"),
		B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS),
	fCurrentDisk(NULL)
{
}


GuideWindow::~GuideWindow()
{
	delete fCurrentDisk;
}


void
GuideWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
GuideWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	Hide();
	return false;
}
