/*
 * Copyright 2006-2007, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef ADD_ITEMS_COMMAND_H
#define ADD_ITEMS_COMMAND_H


#include "Command.h"
#include "Container.h"
#include "IconBuild.h"


_BEGIN_ICON_NAMESPACE
	class BReferenceable;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


/*! Adds items to a \c Container.
	
	\note This class should be subclassed and the \c GetName member overridden.
*/
template<class Type>
class AddItemsCommand : public Command {
 public:
								AddItemsCommand(
									Container<Type>* container,
									Type** const items,
									int32 count,
									bool ownsItems,
									int32 index);
	virtual						~AddItemsCommand();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 protected:
			Container<Type>*	fContainer;
			Type**				fItems;
			int32				fCount;
			bool				fOwnsItems;
			int32				fIndex;
			bool				fItemsAdded;
};

#endif // ADD_ITEMS_COMMAND_H
