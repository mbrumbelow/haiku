/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#include "TMouseSettings.h"

#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <String.h>
#include <View.h>


TMouseSettings::TMouseSettings(BString name)
	:
	MouseSettings(),
	fName(name)
{
	if (_RetrieveSettings() != B_OK)
		Defaults();

	_StoreOriginalSettings();
}


TMouseSettings::TMouseSettings(mouse_settings settings, BString name)
	:
	MouseSettings(&settings),
	fName(name)
{
	_StoreOriginalSettings();
}


TMouseSettings::~TMouseSettings()
{
}


status_t
TMouseSettings::_RetrieveSettings()
{
	// retrieve current values
	mouse_settings settings;
	if (get_mouse_map(&settings.map) != B_OK)
		return B_ERROR;
	if (get_click_speed(&settings.click_speed) != B_OK)
		return B_ERROR;
	if (get_mouse_speed(fName, &settings.accel.speed) != B_OK)
		return B_ERROR;
	if (get_mouse_acceleration(fName, &settings.accel.accel_factor) != B_OK)
		return B_ERROR;
	if (get_mouse_type(fName, &settings.type) != B_OK)
		return B_ERROR;

	MouseSettings::SetMouseType(settings.type);
	MouseSettings::SetClickSpeed(settings.click_speed);
	MouseSettings::SetMouseSpeed(settings.accel.speed);
	MouseSettings::SetAccelerationFactor(settings.accel.accel_factor);
	MouseSettings::SetMapping(settings.map);

	MouseSettings::SetMouseMode(mouse_mode());
	MouseSettings::SetFocusFollowsMouseMode(focus_follows_mouse_mode());
	MouseSettings::SetAcceptFirstClick(accept_first_click());

	return B_OK;
}


// Checks if the settings are different than the system defaults
bool
TMouseSettings::IsDefaultable()
{
	const mouse_settings* settings = GetSettings();

	if (settings->click_speed != kDefaultClickSpeed
		|| settings->accel.speed != kDefaultMouseSpeed
		|| settings->type != kDefaultMouseType
		|| settings->accel.accel_factor != kDefaultAccelerationFactor
		|| MouseMode() != B_NORMAL_MOUSE
		|| FocusFollowsMouseMode() != B_NORMAL_FOCUS_FOLLOWS_MOUSE
		|| AcceptFirstClick() != kDefaultAcceptFirstClick) {
			return true;
	}

	for (int i = 0; i < kDefaultMouseType; i++) {
		if (settings->map.button[i] != (uint32)B_MOUSE_BUTTON(i + 1))
			return true;
	}

	return false;
}


//	Reverts to the active settings at program startup
void
TMouseSettings::Revert()
{
	SetClickSpeed(fOriginalSettings.click_speed);
	SetMouseSpeed(fOriginalSettings.accel.speed);
	SetMouseType(fOriginalSettings.type);
	SetAccelerationFactor(fOriginalSettings.accel.accel_factor);
	SetMouseMode(fOriginalMode);
	SetFocusFollowsMouseMode(fOriginalFocusFollowsMouseMode);
	SetAcceptFirstClick(fOriginalAcceptFirstClick);

	SetMapping(fOriginalSettings.map);
}


// Checks if the settings are different then the original settings
bool
TMouseSettings::IsRevertable()
{
	const mouse_settings* settings = GetSettings();

	if (settings->click_speed != fOriginalSettings.click_speed
		|| settings->accel.speed != fOriginalSettings.accel.speed
		|| settings->type != fOriginalSettings.type
		|| settings->accel.accel_factor != fOriginalSettings.accel.accel_factor
		|| MouseMode() != fOriginalMode
		|| FocusFollowsMouseMode() != fOriginalFocusFollowsMouseMode
		|| AcceptFirstClick() != fOriginalAcceptFirstClick) {
			return true;
	}

	for (int i = 0; i < fOriginalSettings.type; i++) {
		if (settings->map.button[i] != fOriginalSettings.map.button[0])
			return true;
	}

	return false;
}


void
TMouseSettings::SetMouseType(int32 type)
{
	if (set_mouse_type(fName, type) == B_OK)
		MouseSettings::SetMouseType(type);
}


void
TMouseSettings::SetClickSpeed(bigtime_t clickSpeed)
{
	if (set_click_speed(clickSpeed) == B_OK)
		MouseSettings::SetClickSpeed(clickSpeed);
}


void
TMouseSettings::SetMouseSpeed(int32 speed)
{
	if (set_mouse_speed(fName, speed) == B_OK)
		MouseSettings::SetMouseSpeed(speed);
}


void
TMouseSettings::SetAccelerationFactor(int32 factor)
{
	if (set_mouse_acceleration(fName, factor) == B_OK)
		MouseSettings::SetAccelerationFactor(factor);
}


void
TMouseSettings::SetMapping(int32 index, uint32 button)
{
	MouseSettings::SetMapping(index, button);
	mouse_map map;
	Mapping(map);
	set_mouse_map(&map);
}


void
TMouseSettings::SetMapping(mouse_map& map)
{
	if (set_mouse_map(&map) == B_OK)
		MouseSettings::SetMapping(map);
}


void
TMouseSettings::SetMouseMode(mode_mouse mode)
{
	set_mouse_mode(mode);
	MouseSettings::SetMouseMode(mode);
}


void
TMouseSettings::SetFocusFollowsMouseMode(mode_focus_follows_mouse mode)
{
	set_focus_follows_mouse_mode(mode);
	MouseSettings::SetFocusFollowsMouseMode(mode);
}


void
TMouseSettings::SetAcceptFirstClick(bool accept_first_click)
{
	set_accept_first_click(accept_first_click);
	MouseSettings::SetAcceptFirstClick(accept_first_click);
}


void
TMouseSettings::_StoreOriginalSettings()
{
	fOriginalSettings = *GetSettings();
	fOriginalMode = MouseMode();
	fOriginalFocusFollowsMouseMode = FocusFollowsMouseMode();
	fOriginalAcceptFirstClick = AcceptFirstClick();
}


MouseSettings*
TMouseSettingsFactory(const BString& name, const mouse_settings* settings)
{
	if (settings == NULL)
		return new(std::nothrow) TMouseSettings(name);
	return new(std::nothrow) TMouseSettings(*settings, name);
}
