/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "Extent.h"


uint32
hashfunction(const char* name, int length)
{
	uint32 hashVal = 0;

	int lengthCovered = 0;
	int index = 0;
	if (length >= 4) {
		for (; index <= length; index+=4)
		{
			lengthCovered = index;
			hashVal = (name[index] << 21) ^ (name[index+1] << 14) ^ (name[index+2] << 7)
				^ (name[index+3] << 0) ^ ((hashVal << 28) | (hashVal >> (4)));
		}
	}

	int leftToCover = length - lengthCovered;
	if (leftToCover == 3) {
		hashVal = (name[index] << 14) ^ (name[index+1] << 7) ^ (name[index+2] << 0)
			^ ((hashVal << 21) | (hashVal >> (11)));
	}
	if (leftToCover == 2) {
		hashVal = (name[index] << 7) ^ (name[index+1] << 0)
				^ ((hashVal << 14) | (hashVal >> (32 - 14)));
	}
	if (leftToCover == 1) {
		hashVal = (name[index] << 0) ^ ((hashVal << 7) | (hashVal >> (32 - 7)));
	}

	return hashVal;
}


Extent::Extent(Inode* inode)
	:
	fInode(inode)
{
}


void
Extent::FillMapEntry(void* pointerToMap)
{
	uint64 firstHalf = *((uint64*)pointerToMap);
	uint64 secondHalf = *((uint64*)pointerToMap + 1);
		//dividing the 128 bits into 2 parts.
	firstHalf = B_BENDIAN_TO_HOST_INT64(firstHalf);
	secondHalf = B_BENDIAN_TO_HOST_INT64(secondHalf);
	fMap->br_state = (firstHalf >> 63);
	fMap->br_startoff = (firstHalf & MASK(63)) >> 9;
	fMap->br_startblock = ((firstHalf & MASK(9)) << 43) | (secondHalf >> 21);
	fMap->br_blockcount = (secondHalf & MASK(21));
	TRACE("Extent::Init: startoff:(%ld), startblock:(%ld), blockcount:(%ld),"
			"state:(%d)\n",
		fMap->br_startoff,
		fMap->br_startblock,
		fMap->br_blockcount,
		fMap->br_state
		);
}


status_t
Extent::Init()
{
	fMap = new(std::nothrow) ExtentMapEntry;
	if (fMap == NULL)
		return B_NO_MEMORY;

	ASSERT(BlockType() == true);
	void* pointerToMap = DIR_DFORK_PTR(fInode->Buffer());
	FillMapEntry(pointerToMap);

	return B_NOT_SUPPORTED;
}


ExtentBlockTail*
Extent::BlockTail(ExtentDataHeader* header)
{
	return (ExtentBlockTail*)
		((char*)header + fInode->DirBlockSize() - sizeof(ExtentBlockTail));
}


ExtentLeafEntry*
Extent::BlockFirstLeaf(ExtentBlockTail* tail)
{
	return (ExtentLeafEntry*)tail - B_BENDIAN_TO_HOST_INT32(tail->count);
}


bool
Extent::BlockType()
{
	bool status = true;
	if (fInode->NoOfBlocks() != 1)
		status = false;
	if (fInode->Size() != fInode->DirBlockSize())
		status = false;
	return status;
}

