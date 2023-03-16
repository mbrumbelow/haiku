/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2012-2020 Haiku, Inc. All rights reserved.
 * Copyright 2022 Pascal R. G. Abresch <nep@packageloss.eu>
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		John Scipione, jscipione@gmail.com
 *		Pascal R. G. Abresch, nep@packageloss.eu
 *
 * Based on the HaikuControllook
 */

#include "ControlLook.h"
#include "DHMOControllook.h"
#include <GradientRadial.h>
#include <GradientLinear.h>
#include <Region.h>
#include <View.h>

extern "C" BControlLook* (instantiate_control_look)(image_id id)
{
	return new (std::nothrow)BPrivate::DHMOControlLook();
}

namespace BPrivate {


DHMOControlLook::DHMOControlLook()
	: HaikuControlLook()
{
}


DHMOControlLook::~DHMOControlLook()
{
}


void
DHMOControlLook::DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	float radius = 4.0f * (be_plain_font->Size() / 12.f);
	HaikuControlLook::_DrawButtonBackground(view, rect, updateRect, radius, radius, radius, radius,
		base, false, flags, borders, orientation);
}


void
DHMOControlLook::DrawButtonFrame(BView* view, BRect& rect, const BRect& updateRect,
	const rgb_color& base, const rgb_color& background, uint32 flags,
	uint32 borders)
{
	float radius = 4.0f * (be_plain_font->Size() / 12.f);
	HaikuControlLook::_DrawButtonFrame(view, rect, updateRect, radius, radius, radius, radius, base,
		background, 1.0, 1.0, flags, borders);
}


void
DHMOControlLook::DrawButtonWithPopUpBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	float radius = 4.0f * (be_plain_font->Size() / 12.f);
	HaikuControlLook::_DrawButtonBackground(view, rect, updateRect, radius, radius, radius, radius,
		base, true, flags, borders, orientation);
}


void
DHMOControlLook::DrawArrowShapeSharp(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 direction,
	uint32 flags, float tint)
{
	BPoint tri1, tri2, tri3;
	float hInset = rect.Width() / 4;
	float vInset = rect.Height() / 4;
	rect.InsetBy(hInset, vInset);

	switch (direction) {
		case B_LEFT_ARROW:
			tri1.Set(rect.right, rect.top + (rect.Height() / 7));
			tri2.Set(rect.right - rect.Width() / 1.33,
				(rect.top + rect.bottom + 1) / 2);
			tri3.Set(rect.right, rect.bottom + 1 - (rect.Height() / 7));
			break;
		case B_RIGHT_ARROW:
			tri1.Set(rect.left + 1, rect.bottom + 1 - (rect.Height() / 7));
			tri2.Set(rect.left + 1 + rect.Width() / 1.33,
				(rect.top + rect.bottom + 1) / 2);
			tri3.Set(rect.left + 1, rect.top + (rect.Height() / 7));
			break;
		case B_UP_ARROW:
			tri1.Set(rect.left + (rect.Width() / 7), rect.bottom);
			tri2.Set((rect.left + rect.right + 1) / 2,
				rect.bottom - rect.Height() / 1.33);
			tri3.Set(rect.right + 1 - (rect.Width() / 7), rect.bottom);
			break;
		case B_DOWN_ARROW:
		default:
			tri1.Set(rect.left + (rect.Width() / 7), rect.top + 1);
			tri2.Set((rect.left + rect.right + 1) / 2,
				rect.top + 1 + rect.Height() / 1.33);
			tri3.Set(rect.right + 1 - (rect.Width() / 7), rect.top + 1);
			break;
		case B_LEFT_UP_ARROW:
			tri1.Set(rect.left, rect.bottom);
			tri2.Set(rect.left, rect.top);
			tri3.Set(rect.right - 1, rect.top);
			break;
		case B_RIGHT_UP_ARROW:
			tri1.Set(rect.left + 1, rect.top);
			tri2.Set(rect.right, rect.top);
			tri3.Set(rect.right, rect.bottom);
			break;
		case B_RIGHT_DOWN_ARROW:
			tri1.Set(rect.right, rect.top);
			tri2.Set(rect.right, rect.bottom);
			tri3.Set(rect.left + 1, rect.bottom);
			break;
		case B_LEFT_DOWN_ARROW:
			tri1.Set(rect.right - 1, rect.bottom);
			tri2.Set(rect.left, rect.bottom);
			tri3.Set(rect.left, rect.top);
			break;
	}

	BShape arrowShape;
	arrowShape.MoveTo(tri1);
	arrowShape.LineTo(tri2);
	arrowShape.LineTo(tri3);

	if ((flags & B_DISABLED) != 0)
		tint = (tint + B_NO_TINT) / 2;

	view->SetHighColor(tint_color(base, tint));

	float penSize = view->PenSize();
	drawing_mode mode = view->DrawingMode();

	view->MovePenTo(BPoint(0, 0));

	view->SetPenSize(ceilf(hInset / 2.0));
	view->SetDrawingMode(B_OP_OVER);
	view->StrokeShape(&arrowShape);

	view->SetPenSize(penSize);
	view->SetDrawingMode(mode);
}


void
DHMOControlLook::DrawScrollBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	view->PushState();
	view->SetDrawingMode(B_OP_OVER);
    view->SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	view->FillRect(rect);

	view->PopState();
}


void
DHMOControlLook::DrawScrollBarThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation, uint32 knobStyle)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	this->DrawScrollBarBackground(view, rect, updateRect, base, flags, orientation);

	view->PushState();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
	BPoint points[3];
	float cornerInset = 7.0f;
	float bezierStrength = 5.0f;
    view->SetHighColor(ui_color(B_SCROLL_BAR_THUMB_COLOR));
    view->MovePenTo(B_ORIGIN);
	rect.OffsetBy(1, 1);
	BShape shape;
	
	shape.MoveTo(BPoint(rect.left + cornerInset, rect.top));
	shape.LineTo(BPoint(rect.right - cornerInset, rect.top));
	points[0] = BPoint(rect.right - bezierStrength,	rect.top);
	points[1] = BPoint(rect.right,					rect.top + bezierStrength);
	points[2] = BPoint(rect.right,					rect.top + cornerInset);
	shape.BezierTo(points);
	shape.LineTo(BPoint(rect.right, rect.bottom - cornerInset));
	points[0] = BPoint(rect.right,					rect.bottom - bezierStrength);
	points[1] = BPoint(rect.right - bezierStrength,	rect.bottom);
	points[2] = BPoint(rect.right - cornerInset,	rect.bottom);
	shape.BezierTo(points);
	shape.LineTo(BPoint(rect.left + cornerInset, rect.bottom));
	points[0] = BPoint(rect.left + bezierStrength,	rect.bottom);
	points[1] = BPoint(rect.left,					rect.bottom - bezierStrength);
	points[2] = BPoint(rect.left,					rect.bottom - cornerInset);
	shape.BezierTo(points);
	shape.LineTo(BPoint(rect.left, rect.top + cornerInset));
	points[0] = BPoint(rect.left,					rect.top + bezierStrength);
	points[1] = BPoint(rect.left + bezierStrength,	rect.top);
	points[2] = BPoint(rect.left + cornerInset,		rect.top);
	shape.BezierTo(points);
	
	shape.Close();

    //view->FillShape(&shape);
	view->ClipToShape(&shape);
	
	BGradientLinear linear;
	
	if (orientation == B_VERTICAL) {	
		auto middle = rect.top + (rect.bottom - rect.top) /2;
		linear = BGradientLinear(BPoint(rect.left, middle), BPoint(rect.right, middle));
	} else {
		auto middle = rect.left + (rect.right - rect.left) /2;
		linear = BGradientLinear(BPoint(middle, rect.top ), BPoint(middle, rect.bottom));
	}
	
	
	linear.AddColor(tint_color(ui_color(B_SCROLL_BAR_THUMB_COLOR), B_DARKEN_2_TINT), 0);
	linear.AddColor(tint_color(ui_color(B_SCROLL_BAR_THUMB_COLOR), B_DARKEN_2_TINT), 255);
	linear.AddColor(ui_color(B_SCROLL_BAR_THUMB_COLOR), 148);


	
	view->FillRect(rect, linear);
	
	rgb_color highlight = tint_color(ui_color(B_SCROLL_BAR_THUMB_COLOR), B_LIGHTEN_2_TINT);
	highlight.alpha = 100;
	linear.AddColor(highlight, 0);
	linear.AddColor(B_TRANSPARENT_COLOR, 255);
	view->FillRect(rect, linear);
	
	
	BGradientLinear linearHighlight;
	if (orientation == B_VERTICAL) {
		auto middle = rect.left + (rect.right - rect.left) /2;
		linearHighlight = BGradientLinear(BPoint(middle, rect.top ), BPoint(middle, rect.bottom));
	} else {	
		auto middle = rect.top + (rect.bottom - rect.top) /2;
		linearHighlight = BGradientLinear(BPoint(rect.left, middle), BPoint(rect.right, middle));
	}
	
	linearHighlight.AddColor(highlight, 0);
	linearHighlight.AddColor(B_TRANSPARENT_COLOR, 255);
	view->FillRect(rect, linearHighlight);
	view->PopState();
}


void
DHMOControlLook::DrawScrollBarButton(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	int32 direction, orientation orientation, bool down)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	// clip to button
	BRegion buttonRegion(rect);
	view->ConstrainClippingRegion(&buttonRegion);

	bool isEnabled = (flags & B_DISABLED) == 0;

	rgb_color buttonColor = isEnabled ? base
		: tint_color(base, B_LIGHTEN_1_TINT);
	HaikuControlLook::_DrawButtonBackground(view, rect, updateRect, 1.0f, 1.0f, 1.0f, 1.0f,
		buttonColor, false, flags, BControlLook::B_ALL_BORDERS, orientation);

	rect.InsetBy(-1, -1);
	DrawArrowShapeSharp(view, rect, updateRect, ui_color(B_CONTROL_HIGHLIGHT_COLOR)
		, direction, flags, B_LIGHTEN_1_TINT);

	// revert clipping constraints
	BRegion clipping(updateRect);
	view->ConstrainClippingRegion(&clipping);
}


} // namespace BPrivate



