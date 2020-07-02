/*
 * Copyright 2020, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
*/


#ifndef WACOM_SETTINGS_VIEW_H
#define WACOM_SETTINGS_VIEW_H


#include <Box.h>
#include <Bitmap.h>
#include <Button.h>
#include <CheckBox.h>
#include <GroupView.h>
#include <OptionPopUp.h>
#include <PopUpMenu.h>
#include <Slider.h>

class WacomPenView;
class WacomMappingView;


class WacomSettingsView : public BGroupView {
	public:
								WacomSettingsView();
		virtual 				~WacomSettingsView();

		virtual void 			AttachedToWindow();

	private:
		typedef	BBox			inherited;

		WacomPenView*			fWacomPenView;
		WacomMappingView*		fWacomMappingView;
		BButton*				fDefaultButton;
		BButton*				fRevertButton;

};

#endif	/* SETTINGS_VIEW_H */
