/*
 * Copyright 2001-2021, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef TOUCHPAD_SETTINGS_H
#define TOUCHPAD_SETTINGS_H


#include <SupportDefs.h>

#include "kb_mouse_settings.h"


typedef struct {
	bool	scroll_twofinger;
	bool	scroll_twofinger_horizontal;
	float	scroll_rightrange;		// from 0 to 1
	float	scroll_bottomrange;		// from 0 to 1
	uint16	scroll_xstepsize;
	uint16	scroll_ystepsize;
	uint8	scroll_acceleration;	// from 0 to 20

	uint8	tapgesture_sensibility;	// 0 : no tapgesture
									// 20: very light tip is enough (default)
	uint16  padblocker_threshold;	//0 to 100

	mouse_accel	accel;				// standard acceleration
} touchpad_settings;


const static touchpad_settings kDefaultTouchpadSettings = {
	true,
	true,
	0.15,
	0.15,
	7,
	10,
	10,
	20,
	30,
	{
		true,
		65536,
		65536,
	}
};

#define TOUCHPAD_SETTINGS_FILE "Touchpad_settings"

#endif	/* TOUCHPAD_SETTINGS_H */
