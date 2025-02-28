/*
 * Copyright 2025, Haiku, Inc.
 * Authors:
 *     Pawan Yerramilli <me@pawanyerramilli.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "SelectAreaView.h"

#include <Application.h>
#include <Bitmap.h>
#include <Cursor.h>
#include <Screen.h>
#include <Window.h>

#include "Utility.h"


SelectAreaView::SelectAreaView(BBitmap* frame)
	: BView(BScreen().Frame(), "", B_FOLLOW_NONE, B_WILL_DRAW),
	fScreenShot(frame),
	fIsCurrentlyDragging(false),
	fStartCorner(BPoint(0, 0)),
	fEndCorner(BPoint(0, 0))
{
	BCursor crosshairs = BCursor(B_CURSOR_ID_CROSS_HAIR);
	SetViewCursor(&crosshairs);
	MakeFocus(true);
}

void
SelectAreaView::AttachedToWindow()
{
	BBitmap* darkenedShot = new BBitmap(Bounds(), fScreenShot->ColorSpace(), true);
	BView* darkenedView = new BView(darkenedShot->Bounds(), "", B_FOLLOW_NONE, 0);
	darkenedShot->AddChild(darkenedView);
	darkenedShot->Lock();
	darkenedView->DrawBitmap(fScreenShot);
	darkenedView->SetDrawingMode(B_OP_ALPHA);
	darkenedView->SetHighColor(0, 0, 0, 128);
	darkenedView->FillRect(Bounds());
	darkenedView->Sync();
	SetViewBitmap(darkenedShot);
	darkenedShot->RemoveChild(darkenedView);
	delete darkenedView;
	delete darkenedShot;
}

void
SelectAreaView::Draw(BRect updateRect)
{
	if (fIsCurrentlyDragging) {
		BRect frame = _CurrentFrame();
		DrawBitmap(fScreenShot, frame, frame);
	}
}

void
SelectAreaView::KeyDown(const char* bytes, int32 numBytes)
{
	if (bytes[0] == B_ESCAPE)
		be_app->PostMessage(B_QUIT_REQUESTED);
}


void
SelectAreaView::MouseDown(BPoint point)
{
	fIsCurrentlyDragging = true;
	fStartCorner = fEndCorner = point;
}

void
SelectAreaView::MouseMoved(BPoint point, uint32 transit, const BMessage* message)
{
	if (fIsCurrentlyDragging && point != fEndCorner) {
		fEndCorner = point;
		Invalidate();
	}
}

void
SelectAreaView::MouseUp(BPoint point)
{
	if (fIsCurrentlyDragging && fStartCorner != fEndCorner) {
		BMessage message;
		BMessage bitmap;
		message.what = SS_SELECT_AREA_BITMAP;
		BBitmap* cropShot = new BBitmap(_CurrentFrame(true), fScreenShot->ColorSpace(), true);
		BView* cropView = new BView(cropShot->Bounds(), "", B_FOLLOW_NONE, 0);
		cropShot->AddChild(cropView);
		cropShot->Lock();
		cropView->DrawBitmap(fScreenShot, _CurrentFrame(), cropView->Bounds());
		cropView->Sync();
		cropShot->Archive(&bitmap);
		message.AddMessage("selectArea", &bitmap);
		be_app->PostMessage(&message);
		cropShot->RemoveChild(cropView);
		delete cropView;
		delete cropShot;
		Window()->Quit();
	} else
		fIsCurrentlyDragging = false;
}

BRect
SelectAreaView::_CurrentFrame(bool zeroed)
{
	BPoint topLeft = BPoint(min_c(fStartCorner.x, fEndCorner.x),
		min_c(fStartCorner.y, fEndCorner.y));
	BPoint bottomRight = BPoint(max_c(fStartCorner.x, fEndCorner.x),
		max_c(fStartCorner.y, fEndCorner.y));
	return zeroed ? BRect(BPoint(0, 0), bottomRight - topLeft) : BRect(topLeft, bottomRight);
}
