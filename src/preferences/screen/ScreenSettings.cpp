/*
 * Copyright 2001-2015, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Rafael Romo
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "ScreenSettings.h"

#include <File.h>
#include <FindDirectory.h>
#include <Path.h>


static const char* kSettingsFileName = "Screen_data";
static const char* kSettingWindowFrame = "windowFrame";
static const char* kSettingBrightness = "brightness";


ScreenSettings::ScreenSettings()
{
	fWindowFrame.Set(0, 0, 450, 250);
	BPoint offset;
	float brightness = -1;

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(kSettingsFileName);

		BMessage settings;
		BFile file(path.Path(), B_READ_ONLY);
		if (file.InitCheck() == B_OK) {
			if (settings.Unflatten(&file) == B_OK) {
				settings.FindPoint(kSettingWindowFrame, &offset);
				settings.FindFloat(kSettingBrightness, &brightness);
			}
		}
	}

	fWindowFrame.OffsetBy(offset);
	fBrightness = brightness;
}


ScreenSettings::~ScreenSettings()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) < B_OK)
		return;

	path.Append(kSettingsFileName);

	BPoint offset = fWindowFrame.LeftTop();
	BMessage settings;
	settings.AddPoint(kSettingWindowFrame, offset);
	settings.AddFloat(kSettingBrightness, fBrightness);
	
	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);
	if (file.InitCheck() == B_OK)
		settings.Flatten(&file);
}


void
ScreenSettings::SetWindowFrame(BRect frame)
{
	fWindowFrame = frame;
}


void
ScreenSettings::SetBrightness(float brightness)
{
	fBrightness = brightness;
}
