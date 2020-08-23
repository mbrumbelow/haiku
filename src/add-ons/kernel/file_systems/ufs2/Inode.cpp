/*
 * Copyright 2020 Suhel Mehta, mehtasuhel@gmail.com
 * Copyright 2008-2010, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "Inode.h"
#include <string.h>
#include<cmath>


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
	int64_t offset = fs_block * MINBSIZE + offset_in_block * sizeof(fNode);

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
	off_t pos = FindBlock(block_number, block_offset);
	off_t total_block_size = (block_number + 1) * block_size;
	ssize_t remaining_length = total_block_size - file_offset;
	ssize_t length = 0;
		if (file_offset + *_length > (block_number + 1) * block_size) {
			while (true) {
				//code for reading multiple blocks
				length += read_pos(fd, pos, buffer, remaining_length);
				if (size == file_offset + length)
				{
					*_length = length;
					return B_OK;
				}
				block_number++;
				// It means that data is in a indirect block
				pos = FindBlock(block_number,
						block_offset + remaining_length);
				remaining_length = *_length - remaining_length;
			}
		}
		length = read_pos(fd, pos, buffer, *_length);
		*_length = length;
		return B_OK;

	return B_NOT_SUPPORTED;
}


off_t
Inode::FindBlock(off_t block_number, off_t block_offset)
{
	int fd = fVolume->Device();
	ufs2_super_block super_block = fVolume->SuperBlock();
	int32_t block_size = super_block.fs_bsize;
	int32_t fragment_size = super_block.fs_fsize;
	off_t indirect_offset;
	int64_t direct_block;
	off_t number_of_block_pointers = block_size / 8;
	if (block_number < 12) {
		// read from direct block
		return GetBlockPointer(block_number) * fragment_size + block_offset;
	}
	else if (block_number >= 12 && block_number < number_of_block_pointers)
	{
		//read from indirect block
		block_number = block_number - 12;
		indirect_offset = GetIndirectBlockPointer() *
			fragment_size + (8 * block_number);
		read_pos(fd, indirect_offset,
			(void*)&direct_block, sizeof(direct_block));
		return direct_block * fragment_size + block_offset;
	}
	else if ( block_number >= number_of_block_pointers
				&&
			block_number < pow(number_of_block_pointers, 2))
	{
		// Data is in double indirect block
		// Subract the already read blocks
		block_number = block_number - number_of_block_pointers - 12;
		// Calculate indirect block inside double indirect block
		off_t indirect_block_no = block_number / number_of_block_pointers;
		indirect_offset = GetDoubleIndirectBlockPtr() *
			fragment_size + (8 * indirect_block_no);

		int64_t indirect_ptr;
		read_pos(fd, indirect_offset,
			(void*)&indirect_ptr, sizeof(direct_block));

		indirect_offset = indirect_ptr * fragment_size
			+ (8 * (block_number % number_of_block_pointers));

		read_pos(fd, indirect_offset,
			(void*)&direct_block, sizeof(direct_block));

		return direct_block * fragment_size + block_offset;

	} else {
		// Reading from triple indirect block
		off_t block_ptrs = pow(number_of_block_pointers, 2);
		block_number = block_number - block_ptrs
			- number_of_block_pointers - 12;

		// Get double indirect block
		// Double indirect block no
		off_t indirect_block_no = block_number / block_ptrs;

		// offset to double indirect block ptr
		indirect_offset = GetTripleIndirectBlockPtr() *
			fragment_size + (8 * indirect_block_no);
		
		int64_t indirect_ptr;
		// Get the double indirect block ptr
		read_pos(fd, indirect_offset,
			(void*)&indirect_ptr, sizeof(direct_block));

		// Get the indirect block
		// number of indirect block ptr
		indirect_block_no = block_number / number_of_block_pointers;
		// Indirect block ptr offset
		indirect_offset = indirect_ptr * fragment_size
			+ (8 * indirect_block_no);

		read_pos(fd, indirect_offset,
			(void*)&indirect_ptr, sizeof(direct_block));

		// Get direct block pointer
		indirect_offset = indirect_ptr * fragment_size
			+ (8 * (block_number % number_of_block_pointers));

		read_pos(fd, indirect_offset,
			(void*)&direct_block, sizeof(direct_block));

		return direct_block * fragment_size + block_offset;
	}
}


status_t
Inode::ReadLink(char* buffer, size_t *_bufferSize)
{
	strlcpy(buffer, fNode._blocks.symlinkpath, *_bufferSize);
	return B_OK;
}
