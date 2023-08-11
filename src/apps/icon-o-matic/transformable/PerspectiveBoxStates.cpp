/*
 * Copyright 2006-2009, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */

#include "PerspectiveBoxStates.h"

#include <Catalog.h>
#include <Cursor.h>
#include <Locale.h>
#include <View.h>

#include "cursors.h"
#include "PerspectiveBox.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-PerspectiveBoxStates"


using namespace PerspectiveBoxStates;


DragState::DragState(PerspectiveBox* parent)
	:
	fOrigin(0.0, 0.0),
	fParent(parent)
{
}


void
DragState::_SetViewCursor(BView* view, const uchar* cursorData) const
{
	BCursor cursor(cursorData);
	view->SetViewCursor(&cursor);
}


// #pragma mark - DragCornerState


DragCornerState::DragCornerState(PerspectiveBox* parent, BPoint* point)
	:
	DragState(parent),
	fPoint(point)
{
}


void
DragCornerState::SetOrigin(BPoint origin)
{
	DragState::SetOrigin(origin);
}


void
DragCornerState::DragTo(BPoint current, uint32 modifiers)
{
	*fPoint = current;
	fParent->Update(true);
}


void
DragCornerState::UpdateViewCursor(BView* view, BPoint current) const
{
	_SetViewCursor(view, kPathMoveCursor);
}
