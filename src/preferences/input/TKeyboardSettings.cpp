/*
 * Copyright 2004-2006, the Haiku project. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors in chronological order:
 *  mccall@digitalparadise.co.uk
 *  Jérôme Duval
 *  Marcus Overhagen
 */


#include "TKeyboardSettings.h"

#include <File.h>
#include <FindDirectory.h>
#include <Path.h>


TKeyboardSettings::TKeyboardSettings()
{
	fOriginalSettings.key_repeat_rate = KeyboardRepeatRate();
	fOriginalSettings.key_repeat_delay = KeyboardRepeatDelay();
}


TKeyboardSettings::~TKeyboardSettings()
{
}


void
TKeyboardSettings::SetKeyboardRepeatRate(int32 rate)
{
	set_key_repeat_rate(rate);
	KeyboardSettings::SetKeyboardRepeatRate(rate);
}


void
TKeyboardSettings::SetKeyboardRepeatDelay(bigtime_t delay)
{
	set_key_repeat_delay(delay);
	KeyboardSettings::SetKeyboardRepeatDelay(delay);
}


void
TKeyboardSettings::Revert()
{
	SetKeyboardRepeatDelay(fOriginalSettings.key_repeat_delay);
	SetKeyboardRepeatRate(fOriginalSettings.key_repeat_rate);
}


void
TKeyboardSettings::Defaults()
{
	SetKeyboardRepeatDelay(kb_default_key_repeat_delay);
	SetKeyboardRepeatRate(kb_default_key_repeat_rate);
}


bool
TKeyboardSettings::IsDefaultable() const
{
	return KeyboardRepeatDelay() != kb_default_key_repeat_delay
		|| KeyboardRepeatRate() != kb_default_key_repeat_rate;
}
