/*
 * Copyright 2020, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
*/


#ifndef WACOM_MAPPING_VIEW_H
#define WACOM_MAPPING_VIEW_H


#include <Box.h>
#include <Bitmap.h>
#include <Button.h>
#include <CheckBox.h>
#include <OptionPopUp.h>
#include <PopUpMenu.h>
#include <Slider.h>


class WacomMappingView : public BBox {
	public:
								WacomMappingView ();
		virtual 				~WacomMappingView ();

		virtual void 			AttachedToWindow();

	private:
		typedef	BBox			inherited;

			BOptionPopUp*		fOrientationMenu;
			BCheckBox*			fPen;
			BCheckBox*			fMapping;
			BOptionPopUp*		fScreenArea;
			BOptionPopUp*		fTabletArea;
};

#endif	/* WACOM_PEN_VIEW_H */
