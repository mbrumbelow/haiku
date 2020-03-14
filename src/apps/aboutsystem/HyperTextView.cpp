/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT license.
 */
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <TextView.h>

#include "HyperTextView.h"

#include <Cursor.h>
#include <Message.h>
#include <Region.h>
#include <Window.h>

#include <ObjectList.h>


// #pragma mark - HyperTextAction


HyperTextAction::HyperTextAction()
{
}


HyperTextAction::~HyperTextAction()
{
}


void
HyperTextAction::MouseOver(HyperTextView* view, BPoint where, BMessage* message)
{
	font_height hh;
	BFont font;
	rgb_color color;

	BCursor linkCursor(B_CURSOR_ID_FOLLOW_LINK);
	view->SetViewCursor(&linkCursor);

	int32 startOffset = view->StartOffsetActionAt(where);
	int32 endOffset = view->EndOffsetActionAt(where);

	view->GetFontAndColor(startOffset, &font, &color);
	font.GetHeight(&hh);
	int font_leading =(int)hh.leading;
	if (font_leading == 0)
		font_leading = 1;
	int font_ascent = (int)hh.ascent;
	//int font_descent =(int)hh.descent;
	int fFontHeight = font_ascent /*+ font_descent*/ + font_leading + 2;

	BPoint start = view->PointAt(startOffset);
	start.y+=fFontHeight;
	BPoint end = view->PointAt(endOffset);
	end.y+=fFontHeight;
	int startLine = view->LineAt(start);
	int endLine = view->LineAt(end);

	float leftInset;
	view->GetInsets(&leftInset, NULL, NULL, NULL);
	BPoint startTemp = start;

	view ->SetHighColor(color);

	for (int line = startLine;line != endLine;line++) {
		BPoint endTemp =  startTemp;
		endTemp.x = view->LineWidth(line) + leftInset;
		view->StrokeLine(startTemp,endTemp);
		BPoint leftTop = BPoint(fmin(startTemp.x,endTemp.x), fmin(startTemp.y, endTemp.y));
		BPoint rightBottom = BPoint(fmax(startTemp.x, endTemp.x), fmax(startTemp.y, endTemp.y));
		BRect *underlinedRect = new BRect(leftTop,rightBottom);
		view->AddUnderlinedRect(underlinedRect);

		startTemp = view->PointAt(view->OffsetAt(line+1));
		startTemp.y += fFontHeight;
	}
	view->StrokeLine(startTemp,end);
	BPoint leftTop = BPoint(fmin(startTemp.x,end.x), fmin(startTemp.y, end.y));
	BPoint rightBottom = BPoint(fmax(startTemp.x, end.x), fmax(startTemp.y, end.y));
	BRect *underlinedRect = new BRect(leftTop,rightBottom);
	view->AddUnderlinedRect(underlinedRect);

}


void
HyperTextAction::Clicked(HyperTextView* view, BPoint where, BMessage* message)
{
}


// #pragma mark - HyperTextView


struct HyperTextView::ActionInfo {
	ActionInfo(int32 startOffset, int32 endOffset, HyperTextAction* action)
		:
		startOffset(startOffset),
		endOffset(endOffset),
		action(action)
	{
	}

	~ActionInfo()
	{
		delete action;
	}

	static int Compare(const ActionInfo* a, const ActionInfo* b)
	{
		return a->startOffset - b->startOffset;
	}

	static int CompareEqualIfIntersecting(const ActionInfo* a,
		const ActionInfo* b)
	{
		if (a->startOffset < b->endOffset && b->startOffset < a->endOffset)
			return 0;
		return a->startOffset - b->startOffset;
	}

	int32				startOffset;
	int32				endOffset;
	HyperTextAction*	action;
};



class HyperTextView::ActionInfoList
	: public BObjectList<HyperTextView::ActionInfo> {
public:
	ActionInfoList(int32 itemsPerBlock = 20, bool owning = false)
		: BObjectList<HyperTextView::ActionInfo>(itemsPerBlock, owning)
	{
	}
};


HyperTextView::HyperTextView(const char* name, uint32 flags)
	:
	BTextView(name, flags),
	fActionInfos(new ActionInfoList(100, true)),
	fCurrentAction(NULL),
	fUnderlinedRegion(new BRegion())
{
}


HyperTextView::HyperTextView(BRect frame, const char* name, BRect textRect,
		uint32 resizeMask, uint32 flags)
	:
	BTextView(frame, name, textRect, resizeMask, flags),
	fActionInfos(new ActionInfoList(100, true)),
	fCurrentAction(NULL),
	fUnderlinedRegion(new BRegion())
{
}


HyperTextView::~HyperTextView()
{
	delete fActionInfos;
	delete fUnderlinedRegion;
}


void
HyperTextView::MouseDown(BPoint where)
{
	// We eat all mouse button events.

	BTextView::MouseDown(where);
}


void
HyperTextView::MouseUp(BPoint where)
{
	BMessage* message = Window()->CurrentMessage();
	ActionInfo* actionInfo = _ActionAt(where);

	if (actionInfo != NULL && actionInfo->action != NULL) {
		actionInfo->action->Clicked(this, where, message);
	}
	BTextView::MouseUp(where);
}


void
HyperTextView::MouseMoved(BPoint where, uint32 transit,
		const BMessage* dragMessage)
{
	BMessage* message = Window()->CurrentMessage();

	uint32 buttons;
	ActionInfo* actionInfo;
	HyperTextAction* action;

	actionInfo = _ActionAt(where);
	if (message->FindInt32("buttons", (int32*)&buttons) == B_OK
			&& buttons == 0 && actionInfo != NULL && (action = actionInfo->action)  != NULL) {
				if (actionInfo != fCurrentAction)  {
					fCurrentAction = actionInfo;
					Invalidate(fUnderlinedRegion);
				}
				action->MouseOver(this, where, message);
				return;
	}

	Invalidate(fUnderlinedRegion);
	BTextView::MouseMoved(where, transit, dragMessage);
}


void
HyperTextView::AddHyperTextAction(int32 startOffset, int32 endOffset,
		HyperTextAction* action)
{
	if (action == NULL || startOffset >= endOffset) {
		delete action;
		return;
	}

	fActionInfos->BinaryInsert(new ActionInfo(startOffset, endOffset, action),
			ActionInfo::Compare);

	// TODO: Of course we should check for overlaps...
}


void
HyperTextView::InsertHyperText(const char* inText, HyperTextAction* action,
		const text_run_array* inRuns)
{
	int32 startOffset = TextLength();
	Insert(inText, inRuns);
	int32 endOffset = TextLength();

	AddHyperTextAction(startOffset, endOffset, action);
}


void
HyperTextView::InsertHyperText(const char* inText, int32 inLength,
		HyperTextAction* action, const text_run_array* inRuns)
{
	int32 startOffset = TextLength();
	Insert(inText, inLength, inRuns);
	int32 endOffset = TextLength();

	AddHyperTextAction(startOffset, endOffset, action);
}


HyperTextView::ActionInfo*
HyperTextView::_ActionAt(const BPoint& where) const
{
	int32 offset = OffsetAt(where);

	ActionInfo pointer(offset, offset + 1, NULL);

	ActionInfo* action = fActionInfos->BinarySearch(pointer,
			ActionInfo::CompareEqualIfIntersecting);
	if (action != NULL) {
		// verify that the text region was hit
		BRegion textRegion;
		GetTextRegion(action->startOffset, action->endOffset, &textRegion);
		if (textRegion.Contains(where))
			return action;
	}

	return NULL;
}


int32
HyperTextView::StartOffsetActionAt(const BPoint& where) const
{
	int32 offset = OffsetAt(where);

	ActionInfo pointer(offset, offset + 1, NULL);

	const ActionInfo* action = fActionInfos->BinarySearch(pointer,
			ActionInfo::CompareEqualIfIntersecting);
	if (action != NULL) {
		return action->startOffset;
	}

	return -1;
}


int32
HyperTextView::EndOffsetActionAt(const BPoint& where) const
{
	int32 offset = OffsetAt(where);

	ActionInfo pointer(offset, offset + 1, NULL);

	const ActionInfo* action = fActionInfos->BinarySearch(pointer,
			ActionInfo::CompareEqualIfIntersecting);
	if (action != NULL) {
		return action->endOffset;
	}

	return -1;
}


void HyperTextView::AddUnderlinedRect(BRect *underlinedRect) {
	fUnderlinedRegion->Include(*underlinedRect);
}
