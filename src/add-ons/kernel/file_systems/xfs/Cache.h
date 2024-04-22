/*
 * Copyright 2001-2008, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2024, Priyanshu Gupta, guptapiyush.0707@gmail.com.
 * This file may be used under the terms of the MIT License.
 */
#ifndef CACHED_H
#define CACHED_H

//!	interface for the block cache

#include "system_dependencies.h"

#include "Volume.h"


class CachedBlock {
public:
	CachedBlock(Volume* volume);
	CachedBlock(Volume* volume, off_t block);
	~CachedBlock();

	void			Keep();
	void			Unset();

	const uint8*	SetTo(off_t block);
	const uint8*	SetToOffset(off_t block,off_t base, off_t length);

	const uint8*	Block() const { return fBlock; }
	off_t			BlockNumber() const { return fBlockNumber; }

private:
	CachedBlock(const CachedBlock &);
	CachedBlock &operator=(const CachedBlock &);
	// no implementation

protected:
	Volume*			fVolume;
	off_t			fBlockNumber;
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
CachedBlock::Keep()
{
	fBlock = NULL;
}


inline void
CachedBlock::Unset()
{
	if (fBlock != NULL) {
		block_cache_put(fVolume->BlockCache(), fBlockNumber);
		fBlock = NULL;
	}
}


inline const uint8*
CachedBlock::SetTo(off_t block)
{
	Unset();
	fBlockNumber = block;
	fBlock = (uint8 *)block_cache_get(fVolume->BlockCache(), block);
	if(fBlock==NULL){
		ERROR("CACHE::IO ERROR");
		return NULL;
	}
	return fBlock;
}


inline const uint8*
CachedBlock::SetToOffset(off_t blockNum, off_t base, off_t length)
{
	Unset();
	fBlockNumber = blockNum;
	const void* _block;
	status_t st = block_cache_get_etc(fVolume->BlockCache(), blockNum, base, length, &_block);
	if (st != B_OK)
		return nullptr;

	return fBlock = reinterpret_cast<const uint8_t*>(_block);
}
#endif	// CACHED_H
