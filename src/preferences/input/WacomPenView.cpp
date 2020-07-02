/*
 * Copyright 2020, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#include "WacomPenView.h"

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



const uint32 kMsgTipFeel		= 'TPfl';
const uint32 kMsgTipSensitivity		= 'TSss';
const uint32 kMsgTipDoubleClickDistance		= 'TDcd';
const uint32 kMsgEraserSize		= 'ESra';


//	#pragma mark -

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "WacomPenView"

WacomPenView::WacomPenView()
	: BBox("Pen")
{
	fTipFeel = new BSlider("tip feel",
		B_TRANSLATE("Tip Feel"), new BMessage (kMsgTipFeel), 0, 1000, B_HORIZONTAL);
	fTipFeel->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fTipFeel->SetLimitLabels(B_TRANSLATE("Low"),B_TRANSLATE("High"));
	fTipFeel->SetHashMarkCount(7);


	fTipSensitivity = new BSlider("Tip Sensitivity", B_TRANSLATE("Tip Sensitivity"),
		new BMessage (kMsgTipSensitivity), 0, 1000, B_HORIZONTAL);
	fTipSensitivity->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fTipSensitivity->SetLimitLabels(B_TRANSLATE("Low"),B_TRANSLATE("High"));
	fTipSensitivity->SetHashMarkCount(7);

	fTipDoubleClickDistance = new BSlider("Tip Double Click Distance",
		B_TRANSLATE("Tip Double Click Distance"), new BMessage (kMsgTipDoubleClickDistance),  0, 1000, B_HORIZONTAL);
	fTipDoubleClickDistance->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fTipDoubleClickDistance->SetLimitLabels(B_TRANSLATE("Low"),B_TRANSLATE("High"));
	fTipDoubleClickDistance->SetHashMarkCount(7);

	fEraserSize = new BSlider("EraserSize",
		B_TRANSLATE("Eraser Size"), new BMessage (kMsgEraserSize), 0, 1000, B_HORIZONTAL);
	fEraserSize->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fEraserSize->SetLimitLabels(B_TRANSLATE("Normal"),B_TRANSLATE("Big"));
	fEraserSize->SetHashMarkCount(7);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		// Horizontal : A|B
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)

			.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING, 3)
				.AddGroup(B_HORIZONTAL, 0)
					.Add(fTipFeel)
					.End()
				.AddGroup(B_HORIZONTAL, 0)
					.Add(fTipSensitivity)
					.End()
				.AddGroup(B_HORIZONTAL, 0)
					.Add(fTipDoubleClickDistance)
					.End()
				.AddGroup(B_HORIZONTAL, 0)
					.Add(fEraserSize)
					.End()
				.End()
			.End()
		.AddStrut(B_USE_DEFAULT_SPACING);
	SetBorder(B_NO_BORDER);
}


WacomPenView::~WacomPenView()
{

}


void
WacomPenView::AttachedToWindow()
{
//	UpdateFromSettings();
}
