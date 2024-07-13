/*
 * Copyright 2004-2006, the Haiku project. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors in chronological order:
 *  mccall@digitalparadise.co.uk
 *  Jérôme Duval
 *  Marcus Overhagen
 */
#ifndef TKEYBOARD_SETTINGS_H_
#define TKEYBOARD_SETTINGS_H_


#include <SupportDefs.h>

#include "KeyboardSettings.h"
#include "kb_mouse_settings.h"


class TKeyboardSettings : public KeyboardSettings {
public:
						TKeyboardSettings();
						~TKeyboardSettings();

			void		Revert();
			void		Defaults();
			bool		IsDefaultable() const;

			void		SetKeyboardRepeatRate(int32 rate);
			void		SetKeyboardRepeatDelay(bigtime_t delay);

private:
			kb_settings	fOriginalSettings;
};


#endif
