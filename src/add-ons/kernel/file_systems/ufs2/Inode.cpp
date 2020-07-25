/*
 * Copyright 2020 Suhel Mehta, mehtasuhel@gmail.com
 * Copyright 2008-2010, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "Inode.h"
#include <string.h>


#define TRACE_UFS2
#ifdef TRACE_UFS2
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
	ufs2_super_block* superblock = (ufs2_super_block* )&fVolume->SuperBlock();
	int64_t fs_block = ino_to_fsba(superblock, id);
	int64_t offset_in_block = ino_to_fsbo(superblock, id);
	int64_t offset = fs_block * MINBSIZE + offset_in_block * 256;

//	ERROR("%ld\n\n",offset);
	if (read_pos(fd, offset, (void*)&fNode, sizeof(fNode)) != sizeof(fNode))
		ERROR("Inode::Inode(): IO Error\n");


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


status_t
Inode::ReadAt(off_t file_offset, uint8* buffer, size_t* _length)
{
	int fd = fVolume->Device();
	ufs2_super_block super_block = fVolume->SuperBlock();
	int32_t block_size = super_block.fs_bsize;
	int64_t size = Size();
	off_t block_number = file_offset / block_size;
	off_t block_offset = file_offset % block_size;
	off_t pos = GetBlockPointer(block_number) * 4096 + block_offset;

	ssize_t length = 0;
	if (size > 0 && block_number < 12) {
		//data of file is in first twelve direct block only
		off_t total_block_size = (block_number + 1) * block_size;
		ssize_t remaining_length = total_block_size - file_offset;
		if (file_offset + *_length > size) {
			if (file_offset + *_length > (block_number + 1) * block_size) {
				while (true) {
					//code for reading multiple blocks
					char data[remaining_length] = {0};
					strcat((char*)buffer, data);
					if (size == file_offset + length)
					{
						*_length = length;
						block_number = 0;
						return B_OK;
					}
					block_number++;
					pos = GetBlockPointer(block_number) * 4096;
					remaining_length = size - file_offset - remaining_length;
				}
			}
			length = read_pos(fd, pos, buffer, size - file_offset);
			*_length = length;
			block_number = 0;
			return B_OK;
		} else {
			if (file_offset + *_length > (block_number + 1) * block_size) {
				while (true) {
					//code for reading multiple blocks
					length += read_pos(fd, pos, buffer, remaining_length);
					if (length == *_length)
					{
						return B_OK;
					}
					block_number++;
					pos = GetBlockPointer(block_number) * 4096;
					remaining_length = *_length - length;
				}
				block_number++;
			}
			length = read_pos(fd, pos, buffer, *_length);
			if (length > 0) {
				*_length = length;
				return B_OK;
			}
		}
		return length;
	}

	return B_NOT_SUPPORTED;
}
