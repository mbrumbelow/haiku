/*
 * Copyright 2020, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#include "JoystickView.h"

#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Debug.h>
#include <InterfaceDefs.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <SeparatorView.h>
#include <Slider.h>
#include <StringView.h>
#include <TextControl.h>
#include <TranslationUtils.h>
#include <Window.h>

const uint32 kLeftInvertX		= 'BLnx';
const uint32 kLeftInvertY		= 'BLnY';
const uint32 kRightInvertX		= 'BRnx';
const uint32 kRightInvertY		= 'BRnY';
const uint32 kMsgDefaults		= 'BTde';
const uint32 kMsgRevert			= 'BTre';

//	#pragma mark -

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "JoystickView"

JoystickView::JoystickView()
	: BBox("main_view")
{
	fLeftInvertX = new BRadioButton("multiple",
		B_TRANSLATE("Invert X"), new BMessage(kLeftInvertX));
		
	fLeftInvertY = new BRadioButton("multiple",
		B_TRANSLATE("Invert Y"), new BMessage(kLeftInvertY));
		
	fRightInvertX = new BRadioButton("multiple",
		B_TRANSLATE("Invert X"), new BMessage(kRightInvertX));
		
	fRightInvertY = new BRadioButton("multiple",
		B_TRANSLATE("Invert Y"), new BMessage(kRightInvertY));

	fDefaultsButton = new BButton(B_TRANSLATE("Defaults"),
		new BMessage(kMsgDefaults));
//	fDefaultsButton->SetEnabled(fSettings.IsDefaultable());

	fRevertButton = new BButton(B_TRANSLATE("Revert"),
		new BMessage(kMsgRevert));
//	fRevertButton->SetEnabled(false);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
			.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
				.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING)
					.Add(new BStringView("Left Stick",
						B_TRANSLATE("Left Stick")))
					.Add(fLeftInvertX)
					.Add(fLeftInvertY)
					.AddGlue()
					.End()
				.Add(new BSeparatorView(B_VERTICAL))
					.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING, 3)
						.Add(new BStringView("Right Stick",
							B_TRANSLATE("Right Stick")))
						.Add(fRightInvertX)
						.Add(fRightInvertY)
						.End()
					.End()
				.AddStrut(B_USE_DEFAULT_SPACING)
				.Add(new BSeparatorView(B_HORIZONTAL))
					.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING, 3)
					.Add(fDefaultsButton)
					.Add(fRevertButton)
					.AddGlue()
					.End()
			.End();

	SetBorder(B_NO_BORDER);
}


JoystickView::~JoystickView()
{
}




