/*
 * Copyright 2020, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#include "WacomMappingView.h"

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
#include <TextControl.h>
#include <TranslationUtils.h>
#include <Window.h>



const uint32 kMsgOrientation	= 'MPot';
const uint32 kMsgPen			= 'MPen';
const uint32 kMsgMapping		= 'MMap';
const uint32 kMsgScreenArea		= 'SAra';
const uint32 kMsgTabletArea		= 'TAra';

//	#pragma mark -

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "WacomPenView"

WacomMappingView ::WacomMappingView ()
	: BBox("Mapping")
{
	fOrientationMenu = new BOptionPopUp("Orientation", B_TRANSLATE("Orientation"),
			new BMessage(kMsgOrientation));

	fOrientationMenu->AddOption(B_TRANSLATE("A"), 1);
	fOrientationMenu->AddOption(B_TRANSLATE("B"), 2);
	fOrientationMenu->AddOption(B_TRANSLATE("C"), 3);

	fPen = new BCheckBox(B_TRANSLATE("Pen"),
		new BMessage(kMsgPen));
	fMapping = new BCheckBox(
		B_TRANSLATE("Mapping"),
		new BMessage(kMsgMapping));

	fScreenArea = new BOptionPopUp("Screen Area", B_TRANSLATE("Screen Area"),
			new BMessage(kMsgScreenArea));

	fScreenArea->AddOption(B_TRANSLATE("A"), 1);
	fScreenArea->AddOption(B_TRANSLATE("B"), 2);
	fScreenArea->AddOption(B_TRANSLATE("C"), 3);

	fTabletArea = new BOptionPopUp("Tablet Area", B_TRANSLATE("Tablet Area"),
			new BMessage(kMsgTabletArea));

	fTabletArea->AddOption(B_TRANSLATE("A"), 1);
	fTabletArea->AddOption(B_TRANSLATE("B"), 2);
	fTabletArea->AddOption(B_TRANSLATE("C"), 3);


	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		// Horizontal : A|B
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)

			.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING, 3)
				.AddGroup(B_HORIZONTAL, 0)
					.Add(fOrientationMenu)
					.End()
				.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
					.Add(fPen)
					.Add(fMapping)
					.End()
				.AddGroup(B_HORIZONTAL, 0)
					.Add(fScreenArea)
					.End()
				.AddGroup(B_HORIZONTAL, 0)
					.Add(fTabletArea)
					.End()
				.AddGroup(B_HORIZONTAL, 0)
//					.Add(fEraserSize)
					.End()
				.End()
			.End()
		.AddStrut(B_USE_DEFAULT_SPACING);
	SetBorder(B_NO_BORDER);
}


WacomMappingView ::~WacomMappingView ()
{

}


void
WacomMappingView::AttachedToWindow()
{
//	UpdateFromSettings();
}
