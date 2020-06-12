/*
 * Copyright 2020 Suhel Mehta, mehtasuhel@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "DirectoryIterator.h"

#include <stdlib.h>

#include "Inode.h"

#define TRACE_UFS2
#ifdef TRACE_UFS2
#	define TRACE(x...) dprintf("\33[34mufs2:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif

#define ERROR(x...) dprintf("\33[34mufs2:\33[0m " x)


DirectoryIterator::DirectoryIterator(Inode* inode)
	:
	fInode(inode)
{
	TRACE("DirectoryIterator::DirectoryIterator() \n");
}


DirectoryIterator::~DirectoryIterator()
{
}


status_t
DirectoryIterator::InitCheck()
{
	return B_OK;
}


status_t
DirectoryIterator::Lookup(const char* name, size_t length, ino_t* _id)
{
	if (strcmp(name, ".") == 0) {
		*_id = fInode->ID();
		return B_OK;
	} else if (strcmp(name, "..") == 0) {
		if (fInode->ID() == 1)
			*_id = fInode->ID();
	/*	else
			*_id = fInode->Parent();*/

		return B_OK;
	}
}


status_t
DirectoryIterator::GetNext(char* name, size_t* _nameLength, ino_t* _id)
{
	TRACE("In GetNext function\n");
	return B_OK;
}