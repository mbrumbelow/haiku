/*
 * Copyright 2006, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */

#include "StyleContainer.h"

#include <stdio.h>
#include <string.h>

#include <OS.h>

#include "Style.h"

#ifdef ICON_O_MATIC
StyleContainerListener::StyleContainerListener() {}
StyleContainerListener::~StyleContainerListener() {}
#endif


StyleContainer::StyleContainer()
	: Container(true)
{
}


StyleContainer::~StyleContainer()
{
}


// #pragma mark -


bool
StyleContainer::AddStyle(Style* style)
{
	return AddItem(style);
}


bool
StyleContainer::AddStyle(Style* style, int32 index)
{
	return AddItem(style, index);
}


bool
StyleContainer::RemoveStyle(Style* style)
{
	return RemoveItem(style);
}


Style*
StyleContainer::RemoveStyle(int32 index)
{
	return RemoveItem(index);
}


void
StyleContainer::MakeEmpty()
{
	return Container::MakeEmpty();
}


// #pragma mark -


int32
StyleContainer::CountStyles() const
{
	return Container::CountItems();
}


bool
StyleContainer::HasStyle(Style* style) const
{
	return Container::HasItem(style);
}


int32
StyleContainer::IndexOf(Style* style) const
{
	return Container::IndexOf(style);
}


Style*
StyleContainer::StyleAt(int32 index) const
{
	return ItemAt(index);
}


Style*
StyleContainer::StyleAtFast(int32 index) const
{
	return ItemAtFast(index);
}


// #pragma mark -


#ifdef ICON_O_MATIC
bool
StyleContainer::AddListener(StyleContainerListener* listener)
{
	if (listener && !fListeners.HasItem((void*)listener))
		return fListeners.AddItem(listener);
	return false;
}


bool
StyleContainer::RemoveListener(StyleContainerListener* listener)
{
	return fListeners.RemoveItem(listener);
}


// #pragma mark -


void
StyleContainer::ItemAdded(Style* style, int32 index)
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		StyleContainerListener* listener
			= (StyleContainerListener*)listeners.ItemAtFast(i);
		listener->StyleAdded(style, index);
	}
}


void
StyleContainer::ItemRemoved(Style* style)
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		StyleContainerListener* listener
			= (StyleContainerListener*)listeners.ItemAtFast(i);
		listener->StyleRemoved(style);
	}
}
#endif // ICON_O_MATIC

