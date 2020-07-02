/*
 * Copyright 2020, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
*/


#ifndef WACOM_PEN_VIEW_H
#define WACOM_PEN_VIEW_H


#include <Box.h>
#include <Bitmap.h>
#include <Button.h>
#include <CheckBox.h>
#include <OptionPopUp.h>
#include <PopUpMenu.h>
#include <Slider.h>


class WacomPenView : public BBox {
	public:
								WacomPenView();
		virtual 				~WacomPenView();

		virtual void 			AttachedToWindow();

	private:
		typedef	BBox			inherited;

				BSlider*		fTipFeel;
				BSlider*		fTipSensitivity;
				BSlider*		fTipDoubleClickDistance;
				BSlider*		fEraserSize;
};

#endif	/* WACOM_PEN_VIEW_H */
