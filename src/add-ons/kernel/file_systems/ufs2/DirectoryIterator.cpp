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
	fCountDir = 0;
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
	}
	dir_info direct_info;
	int64_t offset = fInode->GetBlockPointer() * MINBSIZE;

	offset = offset + sizeof(dir_info);
	while(_GetNext(name, &length, _id, &offset) != B_OK) {
	}

	return B_OK;
}


status_t
DirectoryIterator::GetNext(char* name, size_t* _nameLength, ino_t* _id)
{
	TRACE("In GetNext function\n");
	int64_t offset = fInode->GetBlockPointer() * MINBSIZE;
	dir direct;
	int fd = fInode->GetVolume()->Device();
	int remainder;
	offset = offset + sizeof(dir_info);
	if (read_pos(fd, offset, &direct, sizeof(dir)) != sizeof(dir)) {
		return B_BAD_DATA;
	}

	for(int i = 0; i < fCountDir; i++)
	{
		remainder = direct.namlen % 4;
		if(remainder != 0) {
			remainder = 4-remainder;
			remainder = direct.namlen + remainder;
		}
		else {
			remainder = direct.namlen + 1;
		}
		offset = offset + 8 + remainder;
		if (read_pos(fd, offset, &direct, sizeof(dir)) != sizeof(dir)) {
		return B_BAD_DATA;
		}
	}

	if (direct.next_ino != 0) {
		TRACE("direct.next_ino %d\n",direct.next_ino);
		remainder = direct.namlen % 4;
		if(remainder != 0) {
			remainder = 4-remainder;
			remainder = direct.namlen + remainder;
		}

		else {
			remainder = direct.namlen + 1;
		}

		strlcpy(name, direct.name, remainder);
		*_id = direct.next_ino;
		*_nameLength = direct.namlen;
		fCountDir++;
		return B_OK;
	}

	fCountDir = 0;
	return B_ENTRY_NOT_FOUND;
}


status_t
DirectoryIterator::_GetNext(const char* name, size_t* _nameLength,
						ino_t* _id, int64_t* offset)
{
	TRACE("In _GetNext function\n");

	dir direct;
	dir_info direct_info;
	int fd = fInode->GetVolume()->Device();
	if (read_pos(fd, *offset - sizeof(dir_info), &direct_info,
		sizeof(dir_info)) != sizeof(dir_info)) {
		return B_BAD_DATA;
	}
	if(strcmp(name, "..") == 0)
	{
		*_id = direct_info.dotdot_ino;
		return B_OK;
	}

	if (read_pos(fd, *offset, &direct, sizeof(dir)) != sizeof(dir)) {
		return B_BAD_DATA;
	}

	int remainder = direct.namlen % 4;
	if(remainder != 0) {
		remainder = 4-remainder;
		remainder = direct.namlen + remainder;
	}
	else {
		remainder = direct.namlen + 1;
	}
	*offset = *offset + 8 + remainder;
	char getname;
	strlcpy(&getname, direct.name, remainder);
	if(strcmp(name, &getname) == 0) {
		*_id = direct.next_ino;
		return B_OK;
	}
	else {
		return B_ENTRY_NOT_FOUND;
	}
}
