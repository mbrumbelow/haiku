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
#include <ScrollBarPrivate.h>
#include <Shape.h>
#include <Size.h>
#include <Window.h>


//	#pragma mark - FakeScrollBar


FakeScrollBar::FakeScrollBar(bool drawArrows, bool doubleArrows,
	uint32 knobStyle, BMessage* message)
	:
	BControl("FakeScrollBar", NULL, message, B_WILL_DRAW | B_NAVIGABLE),
	fDrawArrows(drawArrows),
	fDoubleArrows(doubleArrows),
	fKnobStyle(knobStyle)
{
	// add some height to draw the ring around the scroll bar
	float height = B_H_SCROLL_BAR_HEIGHT + 8;
	SetExplicitMinSize(BSize(200, height));
	SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, height));
}


FakeScrollBar::~FakeScrollBar(void)
{
}


void
FakeScrollBar::Draw(BRect updateRect)
{
	BRect rect(Bounds());

	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);

	uint32 flags = BControlLook::B_SCROLLABLE;
	if (IsFocus())
		flags |= BControlLook::B_FOCUSED;

	if (Value() == B_CONTROL_ON)
		SetHighColor(ui_color(B_CONTROL_MARK_COLOR));
	else
		SetHighColor(base);

	// Draw the selected border (2px)
	StrokeRect(rect);
	rect.InsetBy(1, 1);
	StrokeRect(rect);
	rect.InsetBy(1, 1);

	// draw a 1px gap
	SetHighColor(base);
	StrokeRect(rect);
	rect.InsetBy(1, 1);

	// border color
	rgb_color borderColor = tint_color(base, B_DARKEN_2_TINT);
	rgb_color navigation = ui_color(B_KEYBOARD_NAVIGATION_COLOR);

	// Stroke a dark frame around the scroll bar background independent of
	// enabled state, also handle focus highlighting.
	SetHighColor(IsFocus() ? navigation : borderColor);
	StrokeRect(rect);

	// inset past border
	rect.InsetBy(1, 1);

	// clipping region of button rects
	BRect buttonFrame1(_ButtonRectFor(rect, SCROLL_ARROW_1));
	BRect buttonFrame2(_ButtonRectFor(rect, SCROLL_ARROW_2));
	BRect buttonFrame3(_ButtonRectFor(rect, SCROLL_ARROW_3));
	BRect buttonFrame4(_ButtonRectFor(rect, SCROLL_ARROW_4));

	if (fDrawArrows) {
		// clear BControlLook::B_ACTIVATED flag if set
		flags &= ~BControlLook::B_ACTIVATED;

		_DrawArrowButton(buttonFrame1, updateRect, base, flags,
			BControlLook::B_LEFT_ARROW);
		if (fDoubleArrows) {
			_DrawArrowButton(buttonFrame2, updateRect, base, flags,
				BControlLook::B_RIGHT_ARROW);
			_DrawArrowButton(buttonFrame3, updateRect, base, flags,
				BControlLook::B_LEFT_ARROW);
		}
		_DrawArrowButton(buttonFrame4, updateRect, base, flags,
			BControlLook::B_RIGHT_ARROW);
	}

	float less = floorf(rect.Width() / 4); // thumb takes up 3/4 width
	BRect thumbRect(rect.left + less, rect.top, rect.right - less, rect.bottom);

	float start = fDrawArrows
		? (fDoubleArrows ? buttonFrame2.right + 1 : buttonFrame1.right + 1)
		: rect.left;
	float finish = fDrawArrows
		? (fDoubleArrows ? buttonFrame3.left - 1 : buttonFrame4.left - 1)
		: rect.right;

	BRect beforeThumb = BRect(start, rect.top,
		thumbRect.right + 1, rect.bottom);
	BRect afterThumb = BRect(thumbRect.right + 1, rect.top,
		finish, rect.bottom);

	// draw background besides thumb
	be_control_look->DrawScrollBarBackground(this, beforeThumb,
		updateRect, base, flags, B_HORIZONTAL);
	be_control_look->DrawScrollBarBackground(this, afterThumb,
		updateRect, base, flags, B_HORIZONTAL);

	// draw thumb
	be_control_look->DrawScrollBarThumb(this, thumbRect, updateRect, base,
		flags, B_HORIZONTAL, fKnobStyle);
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
FakeScrollBar::_DrawArrowButton(BRect rect, const BRect& updateRect,
	const rgb_color& base, uint32 flags, int32 direction)
{
	// TODO: why do we need this for the scroll bar to draw right?
	rgb_color arrowColor = tint_color(base, B_LIGHTEN_1_TINT);
	// TODO: Why do we need this negative inset for the arrow to look right?
	BRect arrowRect(rect.InsetByCopy(-1, -1));

	// draw button and arrow
	be_control_look->DrawButtonBackground(this, rect, updateRect, arrowColor,
		flags, BControlLook::B_ALL_BORDERS, B_HORIZONTAL);
	be_control_look->DrawArrowShape(this, arrowRect, arrowRect, base, direction,
		flags, B_DARKEN_4_TINT);
}


BRect
FakeScrollBar::_ButtonRectFor(BRect bounds, int32 button) const
{
	float buttonSize = bounds.Height() + 1;

	BRect rect(bounds.left, bounds.top,
		bounds.left + buttonSize - 1, bounds.top + buttonSize - 1);

	switch (button) {
		case SCROLL_ARROW_1:
		default:
			break;

		case SCROLL_ARROW_2:
			rect.OffsetBy(buttonSize, 0);
			break;

		case SCROLL_ARROW_3:
			rect.OffsetTo(bounds.right - 2 * buttonSize + 1, bounds.top);
			break;

		case SCROLL_ARROW_4:
			rect.OffsetTo(bounds.right - buttonSize + 1, bounds.top);
			break;
	}

	return rect;
}
