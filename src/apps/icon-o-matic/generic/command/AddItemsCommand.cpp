/*
 * Copyright 2006, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */

#include "AddItemsCommand.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Catalog.h>
#include <Locale.h>
#include <Referenceable.h>

#include "Container.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-AddItemsCmd"


using std::nothrow;


template<class Type>
AddItemsCommand<Type>::AddItemsCommand(Container<Type>* container,
								 Type** items,
								 int32 count,
								 bool ownsItems,
								 int32 index)
	: Command(),
	  fContainer(container),
	  fItems(items && count > 0 ? new (nothrow) Type*[count] : NULL),
	  fCount(count),
	  fOwnsItems(ownsItems),
	  fIndex(index),
	  fItemsAdded(false)
{
	if (!fContainer || !fItems)
		return;

	memcpy(fItems, items, sizeof(Type*) * fCount);

	if (!fOwnsItems) {
		// Add references to items
		for (int32 i = 0; i < fCount; i++) {
			if (fItems[i] != NULL)
				fItems[i]->AcquireReference();
		}
	}
}


template<class Type>
AddItemsCommand<Type>::~AddItemsCommand()
{
	if (!fItemsAdded && fItems) {
		for (int32 i = 0; i < fCount; i++) {
			if (fItems[i] != NULL)
				fItems[i]->ReleaseReference();
		}
	}
	delete[] fItems;
}


template<class Type>
status_t
AddItemsCommand<Type>::InitCheck()
{
	return fContainer && fItems ? B_OK : B_NO_INIT;
}


template<class Type>
status_t
AddItemsCommand<Type>::Perform()
{
	// add items to container
	for (int32 i = 0; i < fCount; i++) {
		if (fItems[i] && !fContainer->AddItem(fItems[i], fIndex + i)) {
			// roll back
			for (int32 j = i - 1; j >= 0; j--)
				fContainer->RemoveItem(fItems[j]);
			return B_ERROR;
		}
	}
	fItemsAdded = true;

	return B_OK;
}


template<class Type>
status_t
AddItemsCommand<Type>::Undo()
{
	// remove items from container
	for (int32 i = 0; i < fCount; i++) {
		fContainer->RemoveItem(fItems[i]);
	}
	fItemsAdded = false;

	return B_OK;
}


template<class Type>
void
AddItemsCommand<Type>::GetName(BString& name)
{
	if (fCount > 1)
		name << B_TRANSLATE("Add Items");
	else
		name << B_TRANSLATE("Add Item");
}

