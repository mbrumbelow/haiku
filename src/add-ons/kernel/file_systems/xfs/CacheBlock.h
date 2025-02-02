/*
 * Copyright 2001-2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef CACHED_H
#define CACHED_H

//!	interface for the block cache

#include "system_dependencies.h"

#include "Volume.h"


class CachedBlock
{
public:
							CachedBlock(Volume* volume);
							CachedBlock(Volume* volume, off_t block);
							~CachedBlock();

	void					Unset();

	inline status_t			SetTo(off_t block);
	inline status_t			SetToOffset(off_t block);

	const uint8*			Block() const { return fBlock; }
	off_t					BlockNumber() const { return fBlockNumber; }

private:
	CachedBlock(const CachedBlock &);
	CachedBlock &operator=(const CachedBlock &);
	// no implementation

protected:
	Volume*					fVolume;
	off_t					fBlockNumber;
	const uint8*			fBlock;
};


// inlines

inline
CachedBlock::CachedBlock(Volume* volume)
	:
	fVolume(volume),
	fBlockNumber(0),
	fBlock(NULL)
{
}


inline
CachedBlock::CachedBlock(Volume* volume, off_t block)
	:
	fVolume(volume),
	fBlockNumber(0),
	fBlock(NULL)
{
	SetTo(block);
}


inline
CachedBlock::~CachedBlock()
{
	Unset();
}


inline void
CachedBlock::Unset()
{
	if (fBlock != NULL) {
		block_cache_put(fVolume->BlockCache(), fBlockNumber);
		fBlock = NULL;
	}
}


inline status_t
CachedBlock::SetTo(off_t block)
{
	TRACE("Reading from Block Cache \n");
	status_t status=SetToOffset(block);
	fBlockNumber = block;
	if(status != B_OK) {
		ERROR("CACHE IO ERROR");
		return status;
	}

	return B_OK;
}


inline status_t
CachedBlock::SetToOffset(off_t blockNum)
{
	Unset();
	fBlockNumber = blockNum;
	status_t status = block_cache_get_etc(fVolume->BlockCache(), blockNum, (const void**)&fBlock);
	if (status != B_OK) {
		ERROR("CACHE IO ERROR");
		return status;
	}

	return B_OK;
}

#endif // CACHED_H
