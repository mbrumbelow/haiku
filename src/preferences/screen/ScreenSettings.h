/*
 * Copyright 2001-2018, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Rafael Romo
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef SCREEN_SETTINGS_H
#define SCREEN_SETTINGS_H


#include <Rect.h>


class ScreenSettings {
	public:
		ScreenSettings();
		virtual ~ScreenSettings();

		BRect WindowFrame() const { return fWindowFrame; };
		float Brightness() const { return fBrightness; };
		void SetWindowFrame(BRect);
		void SetBrightness(float);

	private:
		BRect fWindowFrame;
		float fBrightness;
};

#endif	// SCREEN_SETTINGS_H
