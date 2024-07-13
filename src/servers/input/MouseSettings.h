// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2004, Haiku
//
//  This software is part of the Haiku distribution and is covered 
//  by the Haiku license.
//
//
//  File:			MouseSettings.h
//  Authors:		Jérôme Duval,
//					Andrew McCall (mccall@digitalparadise.co.uk),
//					Axel Dörfler (axeld@pinc-software.de)
//  Description:	Input Server
//  Created:		August 29, 2004
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#ifndef MOUSE_SETTINGS_H_
#define MOUSE_SETTINGS_H_

#include <Archivable.h>
#include <InterfaceDefs.h>
#include <kb_mouse_settings.h>
#include <Path.h>
#include <String.h>
#include <SupportDefs.h>

#include <map>


static const bigtime_t kDefaultClickSpeed = 500000;
static const int32 kDefaultMouseSpeed = 65536;
static const int32 kDefaultMouseType = 3;	// 3 button mouse
static const int32 kDefaultAccelerationFactor = 65536;
static const bool kDefaultAcceptFirstClick = true;


class MouseSettings {
	public:
		MouseSettings();
		MouseSettings(const mouse_settings* originalSettings);
		virtual ~MouseSettings();

		void Defaults();
		void Dump();

		int32 MouseType() const { return fSettings.type; }
		virtual void SetMouseType(int32 type);

		bigtime_t ClickSpeed() const;
		virtual void SetClickSpeed(bigtime_t click_speed);

		int32 MouseSpeed() const { return fSettings.accel.speed; }
		virtual void SetMouseSpeed(int32 speed);

		int32 AccelerationFactor() const
			{ return fSettings.accel.accel_factor; }
		virtual void SetAccelerationFactor(int32 factor);

		uint32 Mapping(int32 index) const;
		void Mapping(mouse_map& map) const;
		virtual void SetMapping(int32 index, uint32 button);
		virtual void SetMapping(mouse_map& map);

		mode_mouse MouseMode() const { return fMode; }
		virtual void SetMouseMode(mode_mouse mode);

		mode_focus_follows_mouse FocusFollowsMouseMode() const
			{ return fFocusFollowsMouseMode; }
		virtual void SetFocusFollowsMouseMode(mode_focus_follows_mouse mode);

		bool AcceptFirstClick() const { return fAcceptFirstClick; }
		virtual void SetAcceptFirstClick(bool acceptFirstClick);

		const mouse_settings* GetSettings() { return &fSettings; }

	private:
		mouse_settings	fSettings;

		// FIXME all these extra settings are not specific to each mouse.
		// They should be moved into MultipleMouseSettings directly
		mode_mouse		fMode;
		mode_focus_follows_mouse	fFocusFollowsMouseMode;
		bool			fAcceptFirstClick;
};


typedef MouseSettings* (MouseSettingsFactory)(const BString& name, const mouse_settings* settings);


class MultipleMouseSettings: public BArchivable {
	public:
		MultipleMouseSettings(MouseSettingsFactory* provider = NULL);
		virtual ~MultipleMouseSettings();

		status_t Archive(BMessage* into, bool deep = false) const;

		void Defaults();
		void Dump();
		status_t SaveSettings();


		MouseSettings* AddMouseSettings(BString mouse_name);
		MouseSettings* GetMouseSettings(BString mouse_name);

	private:
		static status_t GetSettingsPath(BPath &path);
		void RetrieveSettings();
		MouseSettingsFactory* _NewMouseSettings;

		typedef std::map<BString, MouseSettings*> mouse_settings_object;
		mouse_settings_object  fMouseSettingsObject;
};

#endif	// MOUSE_SETTINGS_H
