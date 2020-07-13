/*
 * Copyright 2002-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Oliver Siebenmarck
 *		Andrew McCall, mccall@digitalparadise.co.uk
 *		Michael Wilber
 */


#include "TranslatorListView.h"

#include <string.h>

#include <Application.h>
#include <String.h>
#include <TranslatorRoster.h>

static int
compare_items(const void* a, const void* b)
{
	const TranslatorItem* itemA = *(const TranslatorItem**)a;
	const TranslatorItem* itemB = *(const TranslatorItem**)b;

	// Get the first input MIME type
	const translation_format* formatA;
	const translation_format* formatB;
	int32 numA;
	int32 numB;

	static BTranslatorRoster* roster = BTranslatorRoster::Default();
	roster->GetInputFormats(itemA->ID(), &formatA, &numA);
	roster->GetInputFormats(itemB->ID(), &formatB, &numB);

	// Get the MIME supertype
	BString typeA(formatA->MIME);
	int32 slashA = typeA.FindFirst('/');
	typeA.Truncate(slashA);

	BString typeB(formatB->MIME);
	int32 slashB = typeB.FindFirst('/');
	typeB.Truncate(slashB);

	// Compare by supertype, then by name
	int typeDiff = typeA.Compare(typeB);
	if (typeDiff != 0)
		return typeDiff;

	return strcmp(itemA->Text(), itemB->Text());
}


//	#pragma mark -


TranslatorItem::TranslatorItem(translator_id id, const char* name)
	:
	BStringItem(name),
	fID(id)
{
}


TranslatorItem::~TranslatorItem()
{
}


//	#pragma mark -


TranslatorListView::TranslatorListView(const char* name, list_view_type type)
	:
	BListView(name, B_SINGLE_SELECTION_LIST) 
{	
}


TranslatorListView::~TranslatorListView() 
{
}


TranslatorItem*
TranslatorListView::TranslatorAt(int32 index) const
{
	return dynamic_cast<TranslatorItem*>(ItemAt(index));
}


void
TranslatorListView::MessageReceived(BMessage* message) 
{
	uint32 type; 
	int32 count;

	switch (message->what) {
		case B_SIMPLE_DATA:
			// Tell the application object that a
			// file has been dropped on this view
			message->GetInfo("refs", &type, &count); 
			if (count > 0 && type == B_REF_TYPE) {
				message->what = B_REFS_RECEIVED;
				be_app->PostMessage(message);
				Invalidate();
			}
			break;

		default:
			BListView::MessageReceived(message);
			break;
	}
}


void
TranslatorListView::MouseMoved(BPoint point, uint32 transit,
	const BMessage* dragMessage)
{
	if (dragMessage != NULL && transit == B_ENTERED_VIEW) {
		// Draw a red box around the inside of the view
		// to tell the user that this view accepts drops
		SetHighColor(220, 0, 0);
		SetPenSize(4);
		StrokeRect(Bounds());
		SetHighColor(0, 0, 0);
	} else if (dragMessage != NULL && transit == B_EXITED_VIEW)
		Invalidate();
}


void
TranslatorListView::SortItems()
{
	BListView::SortItems(&compare_items);
}

