/*
 * Copyright 2020 Suhel Mehta, mehtasuhel@gmail.com
 * Copyright 2008-2010, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "Inode.h"

#ifdef TRACE_ufs2
#define TRACE(x...) dprintf("\33[34mufs2:\33[0m " x)
#else
#define TRACE(x...) ;
#endif
#define ERROR(x...) dprintf("\33[34mufs2:\33[0m " x)


Inode::Inode(Volume* volume, ino_t id)
	:
	fVolume(volume),
	fID(id),
	fCache(NULL),
	fMap(NULL)
{
	rw_lock_init(&fLock, "ufs2 inode");

	fInitStatus = B_OK;//UpdateNodeFromDisk();
	if (fInitStatus == B_OK) {
		if (!IsDirectory() && !IsSymLink()) {
			fCache = file_cache_create(fVolume->ID(), ID(), Size());
			fMap = file_map_create(fVolume->ID(), ID(), Size());
		}
	}
	int fd = fVolume->Device();

	ERROR("%ld\n", ino_to_fsba((ufs2_super_block*)&fVolume->SuperBlock(), id)*MINBSIZE);

	if (read_pos(fd, 164352, (void*)&fNode, sizeof(fNode)) == sizeof(fNode)) {
		//memcpy((void*)&fNode, fBuffer, 512);
	}
	else
	{
		ERROR("Inode::Inode(): IO Error");
	}
}


Inode::Inode(Volume* volume, ino_t id, const ufs2_inode& item)
	:
	fVolume(volume),
	fID(id),
	fCache(NULL),
	fMap(NULL),
	fInitStatus(B_OK),
	fNode(item)
{
	if (!IsDirectory() && !IsSymLink()) {
		fCache = file_cache_create(fVolume->ID(), ID(), Size());
		fMap = file_map_create(fVolume->ID(), ID(), Size());
	}
}


Inode::Inode(Volume* volume)
	:
	fVolume(volume),
	fID(0),
	fCache(NULL),
	fMap(NULL),
	fInitStatus(B_NO_INIT)
{
	rw_lock_init(&fLock, "ufs2 inode");
}


Inode::~Inode()
{
	TRACE("Inode destructor\n");
	file_cache_delete(FileCache());
	file_map_delete(Map());
	TRACE("Inode destructor: Done\n");
}


status_t
Inode::InitCheck()
{
	return fInitStatus;
}
