/*
 * Copyright 2022, Raghav Sharma, raghavself28@gmail.com
 * Distributed under the terms of the MIT License.
 */


#include "Attribute.h"

#include "LeafAttribute.h"
#include "ShortAttribute.h"


Attribute::~Attribute()
{
}


Attribute*
Attribute::Init(Inode* inode)
{
	if (inode->AttrFormat() == XFS_DINODE_FMT_LOCAL) {
		TRACE("Attribute:Init: LOCAL\n");
		ShortAttribute* shortAttr = new(std::nothrow) ShortAttribute(inode);
		return shortAttr;
	}
	if (inode->AttrFormat() == XFS_DINODE_FMT_EXTENTS) {
		TRACE("Attribute::Init: EXTENTS\n");
		LeafAttribute* fLeafAttr = new(std::nothrow) LeafAttribute(inode);
		if (fLeafAttr == NULL)
			return NULL;

		status_t status = fLeafAttr->Init();

		if (status == B_OK)
			return fLeafAttr;

		// Currently node attributes are not supported return NULL
		return NULL;
	}

	// Invalid format
	return NULL;
}