/*
 * Copyright 2002-2013 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Erik Jaesler <ejakowatz@users.sourceforge.net>
 *		Ithamar R. Adema <ithamar@unet.nl>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "DriveSetup.h"
#include "MainWindow.h"
#include "GuideWindow.h"

#include <stdio.h>
#include <string.h>

#include <Message.h>
#include <File.h>
#include <FindDirectory.h>
#include <Locale.h>
#include <Path.h>


DriveSetup::DriveSetup()
	: BApplication("application/x-vnd.Haiku-DriveSetup"),
	fGuided(false),
	fWindow(NULL),
	fSettings((uint32)0)
{
}


DriveSetup::~DriveSetup()
{
}


void
DriveSetup::ReadyToRun()
{
	if (fGuided) {
		fGuideWindow = new GuideWindow();
		fGuideWindow->Show();
	} else {
		fWindow = new MainWindow();
		if (_RestoreSettings() != B_OK)
			fWindow->ApplyDefaultSettings();
		fWindow->Show();
	}
}


bool
DriveSetup::QuitRequested()
{
	if (fGuided) {
		if (fGuideWindow->Lock()) {
			fGuideWindow->Quit();
			fGuideWindow = NULL;
		}
	} else {
		_StoreSettings();
		if (fWindow->Lock()) {
			fWindow->Quit();
			fWindow = NULL;
		}
	}

	return true;
}


// #pragma mark -


status_t
DriveSetup::_StoreSettings()
{
	status_t ret = B_ERROR;
	if (fWindow == NULL)
		return ret;

	if (fWindow->Lock()) {
		ret = fWindow->StoreSettings(&fSettings);
		fWindow->Unlock();
	}

	if (ret < B_OK) {
		fprintf(stderr, "failed to store settings: %s\n", strerror(ret));
		return ret;
	}

	BFile file;
	ret = _GetSettingsFile(file, true);
	if (ret < B_OK)
		return ret;

	ret = fSettings.Flatten(&file);
	if (ret < B_OK) {
		fprintf(stderr, "failed to flatten settings: %s\n", strerror(ret));
		return ret;
	}

	return B_OK;
}


status_t
DriveSetup::_RestoreSettings()
{
	BFile file;
	if (fWindow == NULL)
		return B_ERROR;

	status_t ret = _GetSettingsFile(file, false);
	if (ret < B_OK)
		return ret;

	ret = fSettings.Unflatten(&file);
	if (ret < B_OK) {
		fprintf(stderr, "failed to unflatten settings: %s\n", strerror(ret));
		return ret;
	}

	ret = fWindow->RestoreSettings(&fSettings);
	if (ret < B_OK) {
		fprintf(stderr, "failed to restore settings: %s\n", strerror(ret));
		return ret;
	}

	return B_OK;
}


status_t
DriveSetup::_GetSettingsFile(BFile& file, bool forWriting) const
{
	BPath path;

	if (fWindow == NULL)
		return B_ERROR;

	status_t ret = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (ret != B_OK) {
		fprintf(stderr, "failed to get user settings folder: %s\n",
			strerror(ret));
		return ret;
	}

	ret = path.Append("DriveSetup");
	if (ret != B_OK) {
		fprintf(stderr, "failed to construct path: %s\n", strerror(ret));
		return ret;
	}

	uint32 writeFlags = B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY;
	uint32 readFlags = B_READ_ONLY;

	ret = file.SetTo(path.Path(), forWriting ? writeFlags : readFlags);
	if (ret != B_OK) {
		if (forWriting) {
			// Only inform of an error if the file was supposed to be written.
			fprintf(stderr, "failed to init settings file: %s\n",
				strerror(ret));
		}
		return ret;
	}

	return B_OK;
}


void
DriveSetup::ArgvReceived(int32 argc, char** argv)
{
	BMessage message(B_REFS_RECEIVED);
	for (int i = 1; i < argc; i++) {
		if (strcmp("-g", argv[i]) == 0
			|| strcmp("--guided", argv[i]) == 0) {
			message.AddBool("guided", true);
			fGuided = true;
			continue;
		}
		const char* drive = argv[i];
		BEntry entry(argv[i], true);
		BPath path;
		if (entry.Exists() && entry.GetPath(&path) == B_OK)
			drive = path.Path();
		message.AddString("drive", drive);
	}
	// Upon program launch, it will buffer a copy of the message, since
	// ArgReceived() is called before ReadyToRun().
	RefsReceived(&message);
}


// #pragma mark -


int
main(int argc, char** argv)
{
	DriveSetup app;
	app.Run();
	return 0;
}
