/*
 * Copyright 2020, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
*/


#ifndef JOYSTICK_VIEW_H
#define JOYSTICK_VIEW_H


#include <Box.h>
#include <Bitmap.h>
#include <Button.h>
#include <CheckBox.h>
#include <OptionPopUp.h>
#include <PopUpMenu.h>
#include <RadioButton.h>
#include <Slider.h>


class JoystickView : public BBox {
	public:
							JoystickView();
		virtual 			~JoystickView();

	private:
		typedef	BBox		inherited;

		BRadioButton*		fLeftInvertX;
		BRadioButton*		fLeftInvertY;
		BRadioButton*		fRightInvertX;
		BRadioButton*		fRightInvertY;
		BButton*			fDefaultsButton;
		BButton*			fRevertButton;
};

#endif	/* JOYSTICK_VIEW_H */
