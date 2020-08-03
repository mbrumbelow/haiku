/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "BPlusTree.h"


TreeDirectory::TreeDirectory(Inode* inode)
	:
	fInode(inode),
	fRoot(NULL),
	fExtents(NULL),
	fSingleDirBlock(NULL),
	fCountOfFilledExtents(0),
	fCurMapIndex(0),
	fOffset(0),
	fCurBlockNumber(0)
{
}


TreeDirectory::~TreeDirectory()
{
	delete fRoot;
	delete fExtents;
	delete fSingleDirBlock;
}


status_t
TreeDirectory::Init()
{
	fRoot = new(std::nothrow) BlockInDataFork;
	if (fRoot == NULL)
		return B_NO_MEMORY;

	fSingleDirBlock = new(std::nothrow) char[fInode->DirBlockSize()];
	if (fSingleDirBlock == NULL)
		return B_NO_MEMORY;

	memcpy((void*)fRoot,
		DIR_DFORK_PTR(fInode->Buffer()), sizeof(BlockInDataFork));

	return B_OK;
}


int
TreeDirectory::BlockLen()
{
	return XFS_BTREE_LBLOCK_SIZE;
}


TreeKey
TreeDirectory::GetKey(int pos)
{
	return BlockLen() + (pos - 1) * XFS_KEY_SIZE;
}


size_t
TreeDirectory::KeySize()
{
	return XFS_KEY_SIZE;
}


size_t
TreeDirectory::PtrSize()
{
	return XFS_PTR_SIZE;
}


TreePointer*
TreeDirectory::GetPtr(int pos, LongBlock* curLongBlock)
{
	size_t availableSpace = fInode->GetVolume()->BlockSize() - BlockLen();
	size_t maxRecords = availableSpace / (KeySize() + PtrSize());
	size_t offsetIntoNode =
		BlockLen() + maxRecords * KeySize() + (pos - 1) * PtrSize();
	return (TreePointer*)((char*)curLongBlock + offsetIntoNode);
}


size_t
TreeDirectory::MaxRecordsPossible(size_t len)
{
	len -= sizeof(BlockInDataFork);
	return len / (KeySize() + PtrSize());
}


status_t
TreeDirectory::GetAllExtents()
{
	xfs_extnum_t noOfExtents = fInode->DataExtentsCount();
	ExtentMapUnwrap* extentsWrapped
		= new(std::nothrow) ExtentMapUnwrap[noOfExtents];
	if (extentsWrapped == NULL)
		return B_NO_MEMORY;

	Volume* volume = fInode->GetVolume();
	uint16 levelsInTree = fRoot->Levels();

	size_t lengthOfDataFork;
	if (fInode->ForkOffset() != 0)
		lengthOfDataFork = fInode->ForkOffset() << 3;
	else
		lengthOfDataFork = volume->InodeSize() - INODE_CORE_UNLINKED_SIZE;

	size_t maxRecords = MaxRecordsPossible(lengthOfDataFork);
	TRACE("Maxrecords: (%d)\n", maxRecords);
	TreePointer* ptrToNode = (TreePointer*)
		((char*)DIR_DFORK_PTR(fInode->Buffer())
			+ sizeof(BlockInDataFork) + maxRecords*KeySize());

	size_t len = volume->BlockSize();
	char node[len];
		// This isn't for a directory block but for one of the tree nodes

	TRACE("levels:(%d)\n", levelsInTree);
	TRACE("Numrecs:(%d)\n", fRoot->NumRecords());

	// Go down the tree by taking the leftest pointer to go to the first leaf
	uint64 fileSystemBlockNo = B_BENDIAN_TO_HOST_INT64(*ptrToNode);
	uint64 readPos = fInode->FileSystemBlockToAddr(fileSystemBlockNo);
	while (levelsInTree !=1) {
		fileSystemBlockNo = B_BENDIAN_TO_HOST_INT64(*ptrToNode);
			// The fs block that contains node at next lower level. Now read.
		readPos = fInode->FileSystemBlockToAddr(fileSystemBlockNo);
		if (read_pos(volume->Device(), readPos, node, len) != len) {
			ERROR("Extent::FillBlockBuffer(): IO Error");
			return B_IO_ERROR;
		}
		LongBlock* curLongBlock = (LongBlock*)node;
		ASSERT(curLongBlock->Magic() == XFS_BMAP_MAGIC);
		ptrToNode = GetPtr(1, curLongBlock);
			// Get's the first pointer. This points to next node.
		levelsInTree--;
	}

	// Next level wil contain leaf nodes. Now Read Directory Buffer
	len = fInode->DirBlockSize();
	if (read_pos(volume->Device(), readPos, fSingleDirBlock, len)
		!= len) {
		ERROR("Extent::FillBlockBuffer(): IO Error");
		return B_IO_ERROR;
	}
	levelsInTree--;
	ASSERT(levelsInTree == 0);

	// We should be at the left most leaf node.
	// This could be a multilevel node type directory
	while (1) {
		// Run till you have leaf blocks to checkout
		char* leafBuffer = fSingleDirBlock;
		ASSERT(((LongBlock*)leafBuffer)->Magic() == XFS_BMAP_MAGIC);
		uint32 offset = sizeof(LongBlock);
		int numRecs = ((LongBlock*)leafBuffer)->NumRecs();

		for (int i = 0; i < numRecs; i++) {
			extentsWrapped[fCountOfFilledExtents].first =
				*(uint64*)(leafBuffer + offset);
			extentsWrapped[fCountOfFilledExtents].second =
				*(uint64*)(leafBuffer + offset + sizeof(uint64));
			offset += sizeof(ExtentMapUnwrap);
			fCountOfFilledExtents++;
		}

		fileSystemBlockNo = ((LongBlock*)leafBuffer)->Right();
		TRACE("Next leaf is at: (%d)\n", fileSystemBlockNo);
		if (fileSystemBlockNo == -1)
			break;
		uint64 readPos = fInode->FileSystemBlockToAddr(fileSystemBlockNo);
		if (read_pos(volume->Device(), readPos, fSingleDirBlock, len)
				!= len) {
				ERROR("Extent::FillBlockBuffer(): IO Error");
				return B_IO_ERROR;
		}
	}
	TRACE("Total covered: (%d)\n", fCountOfFilledExtents);
	return UnWrapExtents(extentsWrapped);
}


status_t
TreeDirectory::FillBuffer(int type, char* blockBuffer, int howManyBlocksFurthur)
{
	TRACE("FILLBUFFER\n");
	ExtentMapEntry map;
	if (type == DATA)
		map = fExtents[fCurMapIndex];
	#if 0
	else if (type == LEAF)
		map = fLeafMap;
	else
		return B_BAD_VALUE;
	#endif

	if (map.br_state !=0)
		return B_BAD_VALUE;

	size_t len = fInode->DirBlockSize();
	if (blockBuffer == NULL) {
		blockBuffer = new(std::nothrow) char[len];
		if (blockBuffer == NULL)
			return B_NO_MEMORY;
	}

	xfs_daddr_t readPos =
		fInode->FileSystemBlockToAddr(map.br_startblock + howManyBlocksFurthur);

	if (read_pos(fInode->GetVolume()->Device(), readPos, blockBuffer, len)
		!= len) {
		ERROR("Extent::FillBlockBuffer(): IO Error");
		return B_IO_ERROR;
	}

	if (type == DATA) {
		fSingleDirBlock = blockBuffer;
		ExtentDataHeader* header = (ExtentDataHeader*) fSingleDirBlock;
		if (B_BENDIAN_TO_HOST_INT32(header->magic) == HEADER_MAGIC) {
			TRACE("DATA BLOCK VALID\n");
		} else {
			TRACE("DATA BLOCK INVALID\n");
			return B_BAD_VALUE;
		}
	} 
	#if 0
	else if (type == LEAF) {
		fLeafBuffer = blockBuffer;
		ExtentLeafHeader* header = (ExtentLeafHeader*) fLeafBuffer;
	}
	#endif
	return B_OK;
}


int
TreeDirectory::EntrySize(int len) const
{
	int entrySize= sizeof(xfs_ino_t) + sizeof(uint8) + len + sizeof(uint16);
			// uint16 is for the tag
	if (fInode->HasFileTypeField())
		entrySize += sizeof(uint8);

	return (entrySize + 7) & -8;
			// rounding off to closest multiple of 8
}


/*
 * Throw in the desired block number and get the index of it
 */
int
TreeDirectory::SearchAndFillMap(int blockNo)
{
	ExtentMapEntry map;
	for (int i = 0; i < fCountOfFilledExtents; i++) {
		map = fExtents[i];
		if (map.br_startoff <= blockNo
			&& (blockNo <= map.br_startoff + map.br_blockcount - 1))
			// Map found
			return i;
	}
	/* unlikely if this is coming from GetNext()
	 * since that'd contain allextent maps of the directory.
	 */
	return -1;
}


status_t
TreeDirectory::GetNext(char* name, size_t* length, xfs_ino_t* ino)
{
	TRACE("TreeDirectory::GetNext\n");
	status_t status;
	if (fExtents == NULL) {
		status = GetAllExtents();
		if (status != B_OK)
			return status;
	}
	status = FillBuffer(DATA, fSingleDirBlock, 0);
	if (status != B_OK)
		return status;

	Volume* volume = fInode->GetVolume();
	void* entry = (void*)((ExtentDataHeader*)fSingleDirBlock + 1);
		// This could be an unused entry so we should check

	uint32 blockNoFromAddress = BLOCKNO_FROM_ADDRESS(fOffset, volume);
	if (fOffset != 0 && blockNoFromAddress == fCurBlockNumber)
		entry = (void*)(fSingleDirBlock + BLOCKOFFSET_FROM_ADDRESS(fOffset, fInode));
		// This gets us a little faster to the next entry

	uint32 curDirectorySize = fInode->Size();
	ExtentMapEntry& map = fExtents[fCurMapIndex];
	while (fOffset != curDirectorySize) {
		blockNoFromAddress = BLOCKNO_FROM_ADDRESS(fOffset, volume);

		TRACE("fOffset:(%d), blockNoFromAddress:(%d)\n",
			fOffset, blockNoFromAddress);
		if (fCurBlockNumber != blockNoFromAddress
			&& blockNoFromAddress > map.br_startoff
			&& blockNoFromAddress
				<= map.br_startoff + map.br_blockcount - 1) {
			// When the block is mapped in the same data
			// map entry but is not the first block
			status = FillBuffer(DATA, fSingleDirBlock,
				blockNoFromAddress - map.br_startoff);
			if (status != B_OK)
				return status;
			entry = (void*)((ExtentDataHeader*)fSingleDirBlock + 1);
			fOffset = fOffset + sizeof(ExtentDataHeader);
			fCurBlockNumber = blockNoFromAddress;
		} else if (fCurBlockNumber != blockNoFromAddress) {
			// When the block isn't mapped in the current data map entry
			fCurMapIndex = SearchAndFillMap(blockNoFromAddress);
			map = fExtents[fCurMapIndex];
			status = FillBuffer(DATA, fSingleDirBlock,
				blockNoFromAddress - map.br_startoff);
			if (status != B_OK)
				return status;
			entry = (void*)((ExtentDataHeader*)fSingleDirBlock + 1);
			fOffset = fOffset + sizeof(ExtentDataHeader);
			fCurBlockNumber = blockNoFromAddress;
		}

		ExtentUnusedEntry* unusedEntry = (ExtentUnusedEntry*)entry;

		if (B_BENDIAN_TO_HOST_INT16(unusedEntry->freetag) == DIR2_FREE_TAG) {
			TRACE("Unused entry found\n");
			fOffset = fOffset + B_BENDIAN_TO_HOST_INT16(unusedEntry->length);
			entry = (void*)
				((char*)entry + B_BENDIAN_TO_HOST_INT16(unusedEntry->length));
			continue;
		}
		ExtentDataEntry* dataEntry = (ExtentDataEntry*) entry;

		uint16 currentOffset = (char*)dataEntry - fSingleDirBlock;
		TRACE("GetNext: fOffset:(%d), currentOffset:(%d)\n",
			BLOCKOFFSET_FROM_ADDRESS(fOffset, fInode), currentOffset);

		if (BLOCKOFFSET_FROM_ADDRESS(fOffset, fInode) > currentOffset) {
			entry = (void*)((char*)entry + EntrySize(dataEntry->namelen));
			continue;
		}

		if (dataEntry->namelen + 1 > *length)
			return B_BUFFER_OVERFLOW;

		fOffset = fOffset + EntrySize(dataEntry->namelen);
		memcpy(name, dataEntry->name, dataEntry->namelen);
		name[dataEntry->namelen] = '\0';
		*length = dataEntry->namelen + 1;
		*ino = B_BENDIAN_TO_HOST_INT64(dataEntry->inumber);

		TRACE("Entry found. Name: (%s), Length: (%ld),ino: (%ld)\n", name,
			*length, *ino);
		return B_OK;
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
TreeDirectory::UnWrapExtents(ExtentMapUnwrap* extentsWrapped)
{
	fExtents = new(std::nothrow) ExtentMapEntry[fCountOfFilledExtents];
	if (fExtents == NULL)
		return B_NO_MEMORY;
	uint64 first, second;

	for (int i = 0; i < fCountOfFilledExtents; i++) {
		first = B_BENDIAN_TO_HOST_INT64(extentsWrapped[i].first);
		second = B_BENDIAN_TO_HOST_INT64(extentsWrapped[i].second);
		fExtents[i].br_state = first >> 63;
		fExtents[i].br_startoff = (first & MASK(63)) >> 9;
		fExtents[i].br_startblock = ((first & MASK(9)) << 43) | (second >> 21);
		fExtents[i].br_blockcount = second & MASK(21);
	}
	delete extentsWrapped;
	extentsWrapped = NULL;

	return B_OK;
}