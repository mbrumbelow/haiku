/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <Debug.h>
#include <CheckBox.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Slider.h>
#include <StringView.h>
#include <TabView.h>

#include "Input_Mouse_Touchpad.h"
#include "InputMouseConstants.h"
// #include "InputMouse.h"
#include "InputWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InputMouseTouchpadView"


InputMouseTouchpadView::InputMouseTouchpadView()
	: BBox("main_view")
{

	fCursorSpeedSlider = new BSlider("cursor_speed",
							B_TRANSLATE("Cursor speed"),
							new BMessage(kMsgCursorSpeed),	0, 1000,
							B_HORIZONTAL);
	fCursorSpeedSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fCursorSpeedSlider->SetHashMarkCount(7);

	fDoubleClickSpeedSlider = new BSlider("double_click_speed",
								B_TRANSLATE("Double Click speed"),
		new BMessage(kMsgDoubleClickSpeed), 0, 1000, B_HORIZONTAL);
	fDoubleClickSpeedSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fDoubleClickSpeedSlider->SetHashMarkCount(7);

	fAcceptfirstclick = new BCheckBox(B_TRANSLATE("Accept First Click"),
						new BMessage(kMsgAcceptFirstClick));

	fFocusMenu = new BPopUpMenu(B_TRANSLATE("Click to focus and raise"));

	const char *focusLabels[] = {B_TRANSLATE_MARK("Click to focus and raise"),
		B_TRANSLATE_MARK("Click to focus"),
		B_TRANSLATE_MARK("Focus follows mouse")};
	const mode_mouse focusModes[] = {B_NORMAL_MOUSE, B_CLICK_TO_FOCUS_MOUSE,
										B_FOCUS_FOLLOWS_MOUSE};

	for (int i = 0; i < 3; i++) {
		BMessage* message = new BMessage(kMsgMouseFocusMode);
		message->AddInt32("mode", focusModes[i]);

		fFocusMenu->AddItem(new BMenuItem(B_TRANSLATE_NOCOLLECT(focusLabels[i]),
			message));
	}

	BMenuField* focusField = new BMenuField(B_TRANSLATE("Focus mode:"),
								fFocusMenu);
	focusField->SetAlignment(B_ALIGN_LEFT);


	/*BTabView* tabView = new BTabView("Mouse",B_WIDTH_FROM_LABEL);
	tabView->SetBorder(B_NO_BORDER);*/

	// fMousePref = new MousePref(fSettings);

	// tabView->AddTab(fMousePref);


BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_WINDOW_SPACING)
	.AddGroup(B_HORIZONTAL,0)
		.Add(fCursorSpeedSlider)
		.Add(fDoubleClickSpeedSlider)
		.End()
	.AddGroup(B_HORIZONTAL,0)
		.Add(focusField)
		.Add(fAcceptfirstclick)
		.End()
	.AddGroup(B_VERTICAL,0)
		// .Add(fMousePref)
		.End()
	.End();
}

InputMouseTouchpadView::~InputMouseTouchpadView()
{
}

void
InputMouseTouchpadView::ShowPref()
{
	fCursorSpeedSlider->Show();
	fDoubleClickSpeedSlider->Show();
	fAcceptfirstclick->Show();
}

void
InputMouseTouchpadView::HidePref()
{
	fCursorSpeedSlider->Hide();
	fDoubleClickSpeedSlider->Hide();
	fAcceptfirstclick->Hide();
}