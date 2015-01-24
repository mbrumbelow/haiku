/*
 *  Copyright 2010-2015 Haiku, Inc. All rights reserved.
 *  Distributed under the terms of the MIT license.
 *
 *	Authors:
 *		DarkWyrm, bpmagic@columbus.rr.com
 *		John Scipione, jscipione@gmail.com
 */


#include "FakeScrollBar.h"

#include <Box.h>
#include <ControlLook.h>
#include <Message.h>
#include <ScrollBar.h>
#include <Shape.h>
#include <Size.h>
#include <Window.h>


typedef enum {
	ARROW_LEFT = 0,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	ARROW_NONE
} arrow_direction;

typedef enum {
	KNOB_NONE = 0,
	KNOB_DOTS,
	KNOB_LINES
} knob_style;


//	#pragma mark - FakeScrollBar


FakeScrollBar::FakeScrollBar(bool drawArrows, bool doubleArrows,
	uint32 knobStyle, BMessage* message)
	:
	BControl("FakeScrollBar", NULL, message, B_WILL_DRAW | B_NAVIGABLE),
	fDrawArrows(drawArrows),
	fDoubleArrows(doubleArrows),
	fKnobStyle(knobStyle)
{
	SetExplicitMinSize(BSize(200, 20));
	SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 20));
}


FakeScrollBar::~FakeScrollBar(void)
{
}


void
FakeScrollBar::Draw(BRect updateRect)
{
	BRect bounds = Bounds();

	rgb_color normal = ui_color(B_PANEL_BACKGROUND_COLOR);

	if (IsFocus()) {
		// draw the focus indicator
		SetHighColor(ui_color(B_NAVIGATION_BASE_COLOR));
		StrokeRect(bounds);
		bounds.InsetBy(1.0, 1.0);

		// Draw the selected border (1px)
		if (Value() == B_CONTROL_ON)
			SetHighColor(ui_color(B_CONTROL_MARK_COLOR));
		else
			SetHighColor(normal);

		StrokeRect(bounds);
		bounds.InsetBy(1.0, 1.0);
	} else {
		// Draw the selected border (2px)
		if (Value() == B_CONTROL_ON)
			SetHighColor(ui_color(B_CONTROL_MARK_COLOR));
		else
			SetHighColor(normal);

		StrokeRect(bounds);
		bounds.InsetBy(1.0, 1.0);
		StrokeRect(bounds);
		bounds.InsetBy(1.0, 1.0);
	}

	// draw a gap (1px)
	SetHighColor(normal);
	StrokeRect(bounds);
	bounds.InsetBy(1.0, 1.0);

	// draw a border around control (1px)
	SetHighColor(tint_color(normal, B_DARKEN_1_TINT));
	StrokeRect(bounds);
	bounds.InsetBy(1.0, 1.0);

	BRect thumbBG = bounds;
	BRect bgRect = bounds;

	if (fDrawArrows) {
		// draw arrows
		SetDrawingMode(B_OP_OVER);

		BRect buttonFrame(bounds.left, bounds.top,
			bounds.left + bounds.Height(), bounds.bottom);

		_DrawArrowButton(ARROW_LEFT, buttonFrame, updateRect);

		if (fDoubleArrows) {
			buttonFrame.OffsetBy(bounds.Height() + 1, 0.0);
			_DrawArrowButton(ARROW_RIGHT, buttonFrame,
				updateRect);

			buttonFrame.OffsetTo(bounds.right - ((bounds.Height() * 2) + 1),
				bounds.top);
			_DrawArrowButton(ARROW_LEFT, buttonFrame,
				updateRect);

			thumbBG.left += bounds.Height() * 2 + 2;
			thumbBG.right -= bounds.Height() * 2 + 2;
		} else {
			thumbBG.left += bounds.Height() + 1;
			thumbBG.right -= bounds.Height() + 1;
		}

		buttonFrame.OffsetTo(bounds.right - bounds.Height(), bounds.top);
		_DrawArrowButton(ARROW_RIGHT, buttonFrame, updateRect);

		SetDrawingMode(B_OP_COPY);

		bgRect = bounds.InsetByCopy(48, 0);
	} else
		bgRect = bounds.InsetByCopy(16, 0);

	// fill background besides the thumb
	BRect leftOfThumb(thumbBG.left, thumbBG.top, bgRect.left - 1,
		thumbBG.bottom);
	BRect rightOfThumb(bgRect.right + 1, thumbBG.top, thumbBG.right,
		thumbBG.bottom);

	be_control_look->DrawScrollBarBackground(this, leftOfThumb,
		rightOfThumb, updateRect, normal, 0, B_HORIZONTAL);

	// Draw scroll thumb

	// fill the clickable surface of the thumb
	rgb_color thumbColor = ui_color(B_SCROLL_BAR_THUMB_COLOR);
	be_control_look->DrawButtonBackground(this, bgRect, updateRect,
		thumbColor, 0, BControlLook::B_ALL_BORDERS, B_HORIZONTAL);

	if (fKnobStyle == KNOB_NONE)
		return;

	rgb_color knobLight = tint_color(thumbColor, B_LIGHTEN_MAX_TINT);
	rgb_color knobDark = tint_color(thumbColor, 1.22);
	BRect rect(bgRect);

	if (fKnobStyle == KNOB_DOTS) {
		// draw dots on the scroll bar thumb
		float hcenter = rect.left + rect.Width() / 2;
		float vmiddle = rect.top + rect.Height() / 2;
		BRect knob(hcenter, vmiddle, hcenter, vmiddle);

		SetHighColor(knobDark);
		FillRect(knob);
		SetHighColor(knobLight);
		FillRect(knob.OffsetByCopy(1, 1));

		float spacer = rect.Height();

		if (rect.left + 3 < hcenter - spacer) {
			SetHighColor(knobDark);
			FillRect(knob.OffsetByCopy(-spacer, 0));
			SetHighColor(knobLight);
			FillRect(knob.OffsetByCopy(-spacer + 1, 1));
		}

		if (rect.right - 3 > hcenter + spacer) {
			SetHighColor(knobDark);
			FillRect(knob.OffsetByCopy(spacer, 0));
			SetHighColor(knobLight);
			FillRect(knob.OffsetByCopy(spacer + 1, 1));
		}
	} else if (fKnobStyle == KNOB_LINES) {
		// draw lines on the scroll bar thumb
		float middle = rect.Width() / 2;

		SetHighColor(knobDark);
		StrokeLine(BPoint(rect.left + middle - 3, rect.top + 2),
			BPoint(rect.left + middle - 3, rect.bottom - 2));
		StrokeLine(BPoint(rect.left + middle, rect.top + 2),
			BPoint(rect.left + middle, rect.bottom - 2));
		StrokeLine(BPoint(rect.left + middle + 3, rect.top + 2),
			BPoint(rect.left + middle + 3, rect.bottom - 2));

		SetHighColor(knobLight);
		StrokeLine(BPoint(rect.left + middle - 2, rect.top + 2),
			BPoint(rect.left + middle - 2, rect.bottom - 2));
		StrokeLine(BPoint(rect.left + middle + 1, rect.top + 2),
			BPoint(rect.left + middle + 1, rect.bottom - 2));
		StrokeLine(BPoint(rect.left + middle + 4, rect.top + 2),
			BPoint(rect.left + middle + 4, rect.bottom - 2));
	}
}


void
FakeScrollBar::MouseDown(BPoint where)
{
	BControl::MouseDown(where);
}


void
FakeScrollBar::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	BControl::MouseMoved(where, transit, dragMessage);
}


void
FakeScrollBar::MouseUp(BPoint where)
{
	SetValue(B_CONTROL_ON);
	Invoke();

	Invalidate();

	BControl::MouseUp(where);
}


void
FakeScrollBar::SetValue(int32 value)
{
	if (value != Value()) {
		BControl::SetValueNoUpdate(value);
		Invalidate();
	}

	if (value == 0)
		return;

	BView* parent = Parent();
	BView* child = NULL;

	if (parent != NULL) {
		// If the parent is a BBox, the group parent is the parent of the BBox
		BBox* box = dynamic_cast<BBox*>(parent);

		if (box && box->LabelView() == this)
			parent = box->Parent();

		if (parent != NULL) {
			BBox* box = dynamic_cast<BBox*>(parent);

			// If the parent is a BBox, skip the label if there is one
			if (box && box->LabelView())
				child = parent->ChildAt(1);
			else
				child = parent->ChildAt(0);
		} else
			child = Window()->ChildAt(0);
	} else if (Window() != NULL)
		child = Window()->ChildAt(0);

	while (child != NULL) {
		FakeScrollBar* scrollbar = dynamic_cast<FakeScrollBar*>(child);

		if (scrollbar != NULL && (scrollbar != this))
			scrollbar->SetValue(B_CONTROL_OFF);
		else {
			// If the child is a BBox, check if the label is a scrollbarbutton
			BBox* box = dynamic_cast<BBox*>(child);

			if (box && box->LabelView()) {
				scrollbar = dynamic_cast<FakeScrollBar*>(box->LabelView());

				if (scrollbar != NULL && (scrollbar != this))
					scrollbar->SetValue(B_CONTROL_OFF);
			}
		}

		child = child->NextSibling();
	}

	//ASSERT(Value() == B_CONTROL_ON);
}


void
FakeScrollBar::SetDoubleArrows(bool doubleArrows)
{
	if (fDoubleArrows == doubleArrows)
		return;

	fDoubleArrows = doubleArrows;
	Invalidate();
}


void
FakeScrollBar::SetKnobStyle(uint32 knobStyle)
{
	if (fKnobStyle == knobStyle)
		return;

	fKnobStyle = knobStyle;
	Invalidate();
}


void
FakeScrollBar::SetFromScrollBarInfo(const scroll_bar_info &info)
{
	if (fDoubleArrows == info.double_arrows && (int32)fKnobStyle == info.knob)
		return;

	fDoubleArrows = info.double_arrows;
	fKnobStyle = info.knob;
	Invalidate();
}


//	#pragma mark - Private methods


void
FakeScrollBar::_DrawArrowButton(int32 direction, BRect rect,
	const BRect& updateRect)
{
	if (!updateRect.Intersects(rect))
		return;

	uint32 flags = 0;

	rgb_color baseColor = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		B_LIGHTEN_1_TINT);

	be_control_look->DrawButtonBackground(this, rect, updateRect, baseColor,
		flags, BControlLook::B_ALL_BORDERS, B_HORIZONTAL);

	rect.InsetBy(-1, -1);
	be_control_look->DrawArrowShape(this, rect, updateRect,
		baseColor, direction, flags, B_DARKEN_MAX_TINT);
}
