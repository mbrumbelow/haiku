/*
 * Copyright 2008-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 * 		Axel Dörfler <axeld@pinc-software.de>
 */


#include "BootManagerWindow.h"

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <TextView.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "BootManager"


static const char* kSignature = "application/x-vnd.Haiku-BootManager";


class BootManager : public BApplication {
public:
								BootManager();

	virtual void				ReadyToRun();
};


BootManager::BootManager()
	:
	BApplication(kSignature)
{
}


void
BootManager::ReadyToRun()
{
	BAlert* alert = new BAlert(B_TRANSLATE("Warning to EFI Booters!"),
		B_TRANSLATE("Please note, using BootManager for EFI booting is "
		"currently not supported."),
		B_TRANSLATE("I'm out of here!"), B_TRANSLATE("Let's Continue!"), NULL,
		B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	alert->SetShortcut(0, B_ESCAPE);

	if (alert->Go() == 0)
	{
		be_app->PostMessage(B_QUIT_REQUESTED);
		return;
	}

	BootManagerWindow* window = new BootManagerWindow();
	window->Show();
}


//	#pragma mark -


int
main(int /*argc*/, char** /*argv*/)
{
	BootManager application;
	application.Run();

	return 0;
}
