/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#ifndef MOUSE_SETTINGS_H
#define MOUSE_SETTINGS_H


#include <InterfaceDefs.h>
#include <Point.h>
#include <SupportDefs.h>

#include "kb_mouse_settings.h"


class BPath;

class MouseSettings {
public:
		MouseSettings();
		~MouseSettings();

		void Revert();
		bool IsRevertable();
		void Defaults();
		bool IsDefaultable();
		void Dump();

		BPoint WindowPosition() const { return fWindowPosition; }
		void SetWindowPosition(BPoint corner);

		int32 MouseType() const { return fSettings.type; }
		void SetMouseType(int32 type);

		uint32 Mapping(int32 index) const;
		void Mapping(mouse_map &map) const;
		void SetMapping(int32 index, uint32 button);
		void SetMapping(mouse_map &map);

private:
		static status_t _GetSettingsPath(BPath &path);
		void _RetrieveSettings();
		status_t _SaveSettings();

		mouse_settings	fSettings, fOriginalSettings;
		BPoint			fWindowPosition;
};

#endif	// MOUSE_SETTINGS_H