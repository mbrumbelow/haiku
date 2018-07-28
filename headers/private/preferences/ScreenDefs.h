/*
 * Copyright 2017, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brian Hill <supernova@warpmail.net>
 */
#ifndef _SCREEN_DEFS_H
#define _SCREEN_DEFS_H


// Message sent to Screen from Backgrounds when updating the background color
static const uint32 UPDATE_DESKTOP_COLOR_MSG = 'udsc';

// Constants used in loading/saving Screen preferences
static const char* const kScreenSettingsFileName = "Screen_data";
static const char* const kScreenSettingWindowFrame = "windowFrame";
static const char* const kScreenSettingBrightness = "brightness";

#endif	// _SCREEN_DEFS_H
