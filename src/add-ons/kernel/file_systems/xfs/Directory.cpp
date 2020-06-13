/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "Directory.h"


DirectoryIterator::DirectoryIterator(Inode* inode)
{
	fInode = inode;
	fShortDir = NULL;
}


DirectoryIterator::~DirectoryIterator()
{
	delete fShortDir;
}


status_t
DirectoryIterator::GetNext(char* name, size_t* length, xfs_ino_t* ino)
{
	if (fInode->Format() == XFS_DINODE_FMT_LOCAL) {
		if (fShortDir == NULL)  {
			fShortDir = new(std::nothrow) ShortDirectory(fInode);
			if (fShortDir == NULL)
				return B_NO_MEMORY;
		}
		status_t status = fShortDir->GetNext(name, length, ino);
		return status;
	}

	//TODO: Reading from extent based dirs
	if (fInode->Format() == XFS_DINODE_FMT_EXTENTS) {
		TRACE("Iterator:GetNext: EXTENTS");
		return B_NOT_SUPPORTED;
	}

	//TODO: Reading from bptree based dirs
	if (fInode->Format() == XFS_DINODE_FMT_BTREE) {
		TRACE("Iterator:GetNext: BTREE");
		return B_NOT_SUPPORTED;
	}

	// Only reaches here if Inode is a device or is corrupt.
	return B_BAD_VALUE;
}


status_t
DirectoryIterator::Lookup(const char* name, size_t length, xfs_ino_t* ino)
{
	if (fInode->Format() == XFS_DINODE_FMT_LOCAL) {
		if (fShortDir == NULL)  {
			fShortDir = new(std::nothrow) ShortDirectory(fInode);
			if (fShortDir == NULL)
				return B_NO_MEMORY;
		}
		status_t status = fShortDir->Lookup(name, length, ino);
		return status;
	}

	//TODO: Reading from extent based dirs
	if (fInode->Format() == XFS_DINODE_FMT_EXTENTS) {
		TRACE("Iterator:Lookup: EXTENTS");
		return B_NOT_SUPPORTED;
	}

	//TODO: Reading from bptree based dirs
	if (fInode->Format() == XFS_DINODE_FMT_BTREE) {
		TRACE("Iterator:Lookup: XFS_DINODE_FMT_BTREE");
		return B_NOT_SUPPORTED;
	}

	return B_BAD_VALUE;
}
