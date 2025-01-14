/*
 * Copyright 2019-2025, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 *		Pawan Yerramilli <me@pawanyerramilli.com>
 */


#include "InputTouchpadPref.h"
#include "InterfaceDefs.h"

#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <List.h>
#include <String.h>

#include <InputServerDevice.h>


TouchpadPref::TouchpadPref(BInputDevice* device)
	:
	fTouchPad(device)
{
	// default center position
	fWindowPosition.x = -1;
	fWindowPosition.y = -1;

	if (LoadSettings() != B_OK)
		Defaults();

	fStartSettings = fSettings;
}


TouchpadPref::~TouchpadPref()
{
	delete fTouchPad;

	SaveSettings();
}


void
TouchpadPref::Revert()
{
	fSettings = fStartSettings;
}


status_t
TouchpadPref::UpdateSettings()
{
	BMessage msg;
	msg.AddBool("scroll_reverse", fSettings.scroll_reverse);
	msg.AddBool("scroll_twofinger", fSettings.scroll_twofinger);
	msg.AddBool(
		"scroll_twofinger_horizontal", fSettings.scroll_twofinger_horizontal);
	msg.AddFloat("scroll_rightrange", fSettings.scroll_rightrange);
	msg.AddFloat("scroll_bottomrange", fSettings.scroll_bottomrange);
	msg.AddInt16("scroll_xstepsize", fSettings.scroll_xstepsize);
	msg.AddInt16("scroll_ystepsize", fSettings.scroll_ystepsize);
	msg.AddInt8("scroll_acceleration", fSettings.scroll_acceleration);
	msg.AddInt8("tapgesture_sensibility", fSettings.tapgesture_sensibility);
	msg.AddInt16("padblocker_threshold", fSettings.padblocker_threshold);

	return fTouchPad->Control(B_SET_TOUCHPAD_SETTINGS, &msg);
}


void
TouchpadPref::Defaults()
{
	fSettings = kDefaultTouchpadSettings;
}


status_t
TouchpadPref::GetSettingsPath(BPath& path)
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status < B_OK)
		return status;

	return path.Append(TOUCHPAD_SETTINGS_FILE);
}


status_t
TouchpadPref::LoadSettings()
{
	BPath path;
	status_t status = GetSettingsPath(path);
	if (status != B_OK)
		return status;

	BFile settingsFile(path.Path(), B_READ_ONLY);
	status = settingsFile.InitCheck();
	if (status != B_OK)
		return status;

	if (settingsFile.Read(&fSettings, sizeof(touchpad_settings))
		!= sizeof(touchpad_settings)) {
		LOG("failed to load settings\n");
		return B_ERROR;
	}

	if (settingsFile.Read(&fWindowPosition, sizeof(BPoint)) != sizeof(BPoint)) {
		LOG("failed to load settings\n");
		return B_ERROR;
	}

	return B_OK;
}


status_t
TouchpadPref::SaveSettings()
{
	BPath path;
	status_t status = GetSettingsPath(path);
	if (status != B_OK)
		return status;

	BFile settingsFile(path.Path(), B_READ_WRITE | B_CREATE_FILE);
	status = settingsFile.InitCheck();
	if (status != B_OK)
		return status;

	if (settingsFile.Write(&fSettings, sizeof(touchpad_settings))
		!= sizeof(touchpad_settings)) {
		LOG("can't save settings\n");
		return B_ERROR;
	}

	if (settingsFile.Write(&fWindowPosition, sizeof(BPoint))
		!= sizeof(BPoint)) {
		LOG("can't save window position\n");
		return B_ERROR;
	}

	return B_OK;
}

void
TouchpadPref::SetSpeed(int32 speed)
{
	int32 value = (int32)pow(2, speed * 6.0 / 1000) * 8192;
		// slow = 8192, fast = 524287; taken from InputMouse.cpp
	if (set_mouse_speed(fTouchPad->Name(), value) == B_OK) {
		fSettings.trackpad_speed = value;
		UpdateSettings();
	}
}

void
TouchpadPref::SetAcceleration(int32 accel)
{
	int32 value = (int32)pow(accel * 4.0 / 1000, 2) * 16384;
		// slow = 0, fast = 262144; taken from InputMouse.cpp
	if (set_mouse_acceleration(fTouchPad->Name(), value) == B_OK) {
		fSettings.trackpad_acceleration = value;
		UpdateSettings();
	}
}