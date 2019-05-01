/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
*/


#include <FindDirectory.h>
#include <File.h>
#include <Path.h>
#include <View.h>

#include <stdio.h>

#include "MouseSettings.h"

// The R5 settings file differs from that of OpenBeOS;
// the latter maps 16 different mouse buttons
#define R5_COMPATIBLE 1

static const int32 kDefaultMouseType = 3;	// 3 button mouse


MouseSettings::MouseSettings()
	:
	fWindowPosition(-1, -1)
{
	_RetrieveSettings();

	fOriginalSettings = fSettings;
}


MouseSettings::~MouseSettings()
{
	_SaveSettings();
}


status_t
MouseSettings::_GetSettingsPath(BPath &path)
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status < B_OK)
		return status;

	path.Append(mouse_settings_file);
	return B_OK;
}


void
MouseSettings::_RetrieveSettings()
{
	// retrieve current values

	if (get_mouse_map(&fSettings.map) != B_OK)
		fprintf(stderr, "error when get_mouse_map\n");
	if (get_mouse_type(&fSettings.type) != B_OK)
		fprintf(stderr, "error when get_mouse_type\n");

	BPath path;
	if (_GetSettingsPath(path) < B_OK)
		return;

	BFile file(path.Path(), B_READ_ONLY);
	if (file.InitCheck() < B_OK)
		return;

#if R5_COMPATIBLE
	const off_t kOffset = sizeof(mouse_settings) - sizeof(mouse_map)
		+ sizeof(int32) * 3;
		// we have to do this because mouse_map counts 16 buttons in OBOS
#else
	const off_t kOffset = sizeof(mouse_settings);
#endif

	if (file.ReadAt(kOffset, &fWindowPosition, sizeof(BPoint))
		!= sizeof(BPoint)) {
		// set default window position (invalid positions will be
		// corrected by the application; the window will be centered
		// in this case)
		fWindowPosition.x = -1;
		fWindowPosition.y = -1;
	}

#ifdef DEBUG
	Dump();
#endif
}


status_t
MouseSettings::_SaveSettings()
{
	BPath path;
	status_t status = _GetSettingsPath(path);
	if (status < B_OK)
		return status;

	BFile file(path.Path(), B_READ_WRITE | B_CREATE_FILE);
	status = file.InitCheck();
	if (status < B_OK)
		return status;

#if R5_COMPATIBLE
	const off_t kOffset = sizeof(mouse_settings) - sizeof(mouse_map)
		+ sizeof(int32) * 3;
	// we have to do this because mouse_map counts 16 buttons in OBOS
#else
	const off_t kOffset = sizeof(mouse_settings);
#endif

	file.WriteAt(kOffset, &fWindowPosition, sizeof(BPoint));

	return B_OK;
}


#ifdef DEBUG
void
MouseSettings::Dump()
{
	printf("type:\t\t%" B_PRId32 " button mouse\n", fSettings.type);
	printf("map:\t\tleft = %" B_PRIu32 " : middle = %" B_PRIu32 " : right = %"
		B_PRIu32 "\n", fSettings.map.button[0], fSettings.map.button[2],
		fSettings.map.button[1]);

	}
#endif


// Resets the settings to the system defaults
void
MouseSettings::Defaults()
{
	SetMouseType(kDefaultMouseType);

	mouse_map map;
	if (get_mouse_map(&map) == B_OK) {
		map.button[0] = B_PRIMARY_MOUSE_BUTTON;
		map.button[1] = B_SECONDARY_MOUSE_BUTTON;
		map.button[2] = B_TERTIARY_MOUSE_BUTTON;
		SetMapping(map);
	}
}


// Checks if the settings are different then the system defaults
bool
MouseSettings::IsDefaultable()
{
	return fSettings.type != kDefaultMouseType
		|| fSettings.map.button[0] != B_PRIMARY_MOUSE_BUTTON
		|| fSettings.map.button[1] != B_SECONDARY_MOUSE_BUTTON
		|| fSettings.map.button[2] != B_TERTIARY_MOUSE_BUTTON;
}


//	Reverts to the active settings at program startup
void
MouseSettings::Revert()
{
	SetMouseType(fOriginalSettings.type);

	SetMapping(fOriginalSettings.map);
}


// Checks if the settings are different then the original settings
bool
MouseSettings::IsRevertable()
{
	return fSettings.type != fOriginalSettings.type
		|| fSettings.map.button[0] != fOriginalSettings.map.button[0]
		|| fSettings.map.button[1] != fOriginalSettings.map.button[1]
		|| fSettings.map.button[2] != fOriginalSettings.map.button[2];
}


void
MouseSettings::SetWindowPosition(BPoint corner)
{
	fWindowPosition = corner;
}


void
MouseSettings::SetMouseType(int32 type)
{
	if (set_mouse_type(type) == B_OK)
		fSettings.type = type;
	else
		fprintf(stderr, "error when set_mouse_type\n");
}

uint32
MouseSettings::Mapping(int32 index) const
{
	return fSettings.map.button[index];
}


void
MouseSettings::Mapping(mouse_map &map) const
{
	map = fSettings.map;
}


void
MouseSettings::SetMapping(int32 index, uint32 button)
{
	fSettings.map.button[index] = button;
	if (set_mouse_map(&fSettings.map) != B_OK)
		fprintf(stderr, "error when set_mouse_map\n");
}


void
MouseSettings::SetMapping(mouse_map &map)
{
	if (set_mouse_map(&map) == B_OK)
		fSettings.map = map;
	else
		fprintf(stderr, "error when set_mouse_map\n");
}
