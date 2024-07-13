/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */
#ifndef TMOUSE_SETTINGS_H
#define TMOUSE_SETTINGS_H


#include <map>

#include <Archivable.h>
#include <Input.h>
#include <InterfaceDefs.h>
#include <Point.h>
#include <String.h>
#include <SupportDefs.h>

#include "MouseSettings.h"
#include "kb_mouse_settings.h"


class TMouseSettings : public MouseSettings {
public:
						TMouseSettings(BString name);
						TMouseSettings(mouse_settings settings, BString name);
						~TMouseSettings();

			void		Revert();
			bool		IsRevertable();
			bool		IsDefaultable();

			void		SetMouseType(int32 type);
			void		SetClickSpeed(bigtime_t click_speed);
			void		SetMouseSpeed(int32 speed);
			void		SetAccelerationFactor(int32 factor);

			void		SetMapping(int32 index, uint32 button);
			void		SetMapping(mouse_map &map);

			void		SetMouseMode(mode_mouse mode);
			void		SetFocusFollowsMouseMode(mode_focus_follows_mouse mode);
			void		SetAcceptFirstClick(bool accept_first_click);

private:
			status_t	_RetrieveSettings();
			void		_StoreOriginalSettings();

private:
			BString						fName;
			mode_mouse					fOriginalMode;
			mode_focus_follows_mouse	fOriginalFocusFollowsMouseMode;
			bool						fOriginalAcceptFirstClick;
			mouse_settings				fOriginalSettings;
};


MouseSettingsFactory TMouseSettingsFactory;

#endif	// TMOUSE_SETTINGS_H
