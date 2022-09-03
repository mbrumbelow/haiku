/*
 * Copyright 2022, Raghav Sharma, raghavself28@gmail.com
 * Distributed under the terms of the MIT License.
 */


#include "NodeAttribute.h"

#include "VerifyHeader.h"


NodeAttribute::NodeAttribute(Inode* inode)
	:
	fInode(inode),
	fName(NULL),
	fLeafBuffer(NULL),
	fNodeBuffer(NULL),
	fCurLeafBuffer(NULL)
{
	fLastEntryOffset = 0;
	fLastNodeOffset = 0;
}


NodeAttribute::NodeAttribute(Inode* inode, const char* name)
	:
	fInode(inode),
	fName(NULL),
	fLeafBuffer(NULL),
	fNodeBuffer(NULL),
	fCurLeafBuffer(NULL)
{
	fLastEntryOffset = 0;
	fLastNodeOffset = 0;
}


NodeAttribute::~NodeAttribute()
{
	delete fMap;
	delete fLeafBuffer;
	delete fNodeBuffer;
	delete fCurLeafBuffer;
}


status_t
NodeAttribute::Init()
{
	status_t status;

	fMap = new(std::nothrow) ExtentMapEntry;
	if (fMap == NULL)
		return B_NO_MEMORY;

	status = FillMapEntry(0);

	if (status != B_OK)
		return status;

    // allocate memory for both Node and Leaf Buffer
	int len = fInode->DirBlockSize();
	fLeafBuffer = new(std::nothrow) char[len];
	if (fLeafBuffer == NULL)
		return B_NO_MEMORY;
	fNodeBuffer = new(std::nothrow) char[len];
	if (fNodeBuffer == NULL)
		return B_NO_MEMORY;

	// fill up Node buffer just one time
	status = FillBuffer(fNodeBuffer, fMap->br_startblock);
	if (status != B_OK)
		return status;

	NodeHeader* header  = CreateNodeHeader(fInode,fNodeBuffer);
	if (header == NULL)
		return B_NO_MEMORY;

	if (!VerifyHeader<NodeHeader>(header, fNodeBuffer, fInode, 0, fMap, ATTR_NODE)) {
		ERROR("Invalid data header");
		delete header;
		return B_BAD_VALUE;
	}
	delete header;

	return B_OK;
}


status_t
NodeAttribute::FillMapEntry(xfs_extnum_t num)
{
	void* AttributeFork = DIR_AFORK_PTR(fInode->Buffer(),
		fInode->CoreInodeSize(), fInode->ForkOffset());

	uint64* pointerToMap = (uint64*)(((char*)AttributeFork + num * EXTENT_SIZE));
	uint64 firstHalf = pointerToMap[0];
	uint64 secondHalf = pointerToMap[1];
		//dividing the 128 bits into 2 parts.

	firstHalf = B_BENDIAN_TO_HOST_INT64(firstHalf);
	secondHalf = B_BENDIAN_TO_HOST_INT64(secondHalf);
	fMap->br_state = firstHalf >> 63;
	fMap->br_startoff = (firstHalf & MASK(63)) >> 9;
	fMap->br_startblock = ((firstHalf & MASK(9)) << 43) | (secondHalf >> 21);
	fMap->br_blockcount = secondHalf & MASK(21);

	return B_OK;
}


xfs_fsblock_t
NodeAttribute::LogicalToFileSystemBlock(uint32 LogicalBlock)
{
	xfs_extnum_t TotalExtents = fInode->AttrExtentsCount();

	// If LogicalBlock is lesser than or equal to ExtentsCount then
	// simply read and return extent block count.
	// else read last Map and add difference of logical block offset
	if (LogicalBlock < (uint32)TotalExtents) {
		FillMapEntry(LogicalBlock);
		return fMap->br_startblock;
	}
	else {
		FillMapEntry(TotalExtents - 1);
		return fMap->br_startblock + LogicalBlock - fMap->br_startoff;
	}
}


status_t
NodeAttribute::FillBuffer(char* buffer, xfs_fsblock_t block)
{
	int len = fInode->DirBlockSize();

	xfs_daddr_t readPos =
		fInode->FileSystemBlockToAddr(block);

	if (read_pos(fInode->GetVolume()->Device(), readPos, buffer, len)
		!= len) {
		ERROR("Extent::FillBlockBuffer(): IO Error");
		return B_IO_ERROR;
	}

	return B_OK;
}


status_t
NodeAttribute::Stat(struct stat& stat)
{
	TRACE("NodeAttribute::Stat\n");

	size_t namelength = strlen(fName);

	status_t status;

	// check if this attribute exists
	status = Lookup(fName, &namelength);

	if(status != B_OK)
		return status;

	// We have valid attribute entry to stat
	if (fLocalEntry != NULL) {
		uint16 valuelen = B_BENDIAN_TO_HOST_INT16(fLocalEntry->valuelen);
		stat.st_type = B_XATTR_TYPE;
		stat.st_size = valuelen + fLocalEntry->namelen;
	} else {
		uint32 valuelen = B_BENDIAN_TO_HOST_INT32(fRemoteEntry->valuelen);
		stat.st_type = B_XATTR_TYPE;
		stat.st_size = valuelen + fRemoteEntry->namelen;
	}

	return B_OK;
}


status_t
NodeAttribute::Read(attr_cookie* cookie, off_t pos, uint8* buffer, size_t* length)
{
	TRACE("NodeAttribute::Read\n");

	if(pos < 0)
		return B_BAD_VALUE;

	size_t namelength = strlen(fName);

	status_t status;

	status = Lookup(fName, &namelength);

	if (status != B_OK)
		return status;

	uint32 LengthToRead = 0;

	if (fLocalEntry != NULL) {
		uint16 valuelen = B_BENDIAN_TO_HOST_INT16(fLocalEntry->valuelen);
		if (pos + *length > valuelen)
			LengthToRead = valuelen - pos;
		else
			LengthToRead = *length;

		char* PtrToOffset = (char*) fLocalEntry + sizeof(uint16)
		+ sizeof(uint8) + fLocalEntry->namelen;

		memcpy(buffer, PtrToOffset, LengthToRead);

		*length = LengthToRead;

		return B_OK;
	} else {
		uint32 valuelen = B_BENDIAN_TO_HOST_INT32(fRemoteEntry->valuelen);
		if (pos + *length > valuelen)
			LengthToRead = valuelen - pos;
		else
			LengthToRead = *length;

		// For remote value blocks, value is stored in seperate file system block
		uint32 blkno = B_BENDIAN_TO_HOST_INT32(fRemoteEntry->valueblk);

		xfs_daddr_t readPos =
			fInode->FileSystemBlockToAddr(blkno);

		if (fInode->Version() == 3)
			pos += sizeof(AttrRemoteHeader);

		readPos += pos;

		// TODO : Implement remote header checks for V5 file system
		if (read_pos(fInode->GetVolume()->Device(), readPos, buffer, LengthToRead)
			!= LengthToRead) {
			ERROR("Extent::FillBlockBuffer(): IO Error");
			return B_IO_ERROR;
		}

		*length = LengthToRead;

		return B_OK;
	}
}


status_t
NodeAttribute::GetNext(char* name, size_t* nameLength)
{
	TRACE("NodeAttribute::GetNext\n");

	NodeHeader* node = CreateNodeHeader(fInode, fNodeBuffer);
	NodeEntry* Nentry = (NodeEntry*)(fNodeBuffer + SizeOfNodeHeader(fInode));

	int TotalNodeEntries = node->Count();

	delete node;

	for (int i = 0; i < TotalNodeEntries; i++) {
		if (i < fLastNodeOffset) {
			Nentry = (NodeEntry*)((char*)Nentry + sizeof(NodeEntry));
			continue;
		}
		fLastNodeOffset = i;

		if (fCurLeafBuffer == NULL) {
			int len = fInode->DirBlockSize();
			fCurLeafBuffer = new(std::nothrow) char[len];
			if (fCurLeafBuffer == NULL)
				return B_NO_MEMORY;
			// First see the leaf block from NodeEntry and logical block offset
			uint32 LogicalBlock = B_BENDIAN_TO_HOST_INT32(Nentry->before);
			// Now calculate File system Block of This logical block
			xfs_fsblock_t block = LogicalToFileSystemBlock(LogicalBlock);
			FillBuffer(fCurLeafBuffer, block);
			fLastEntryOffset = 0;
		}
		AttrLeafHeader* header  = CreateAttrLeafHeader(fInode,fCurLeafBuffer);
		AttrLeafEntry* entry = (AttrLeafEntry*)(fCurLeafBuffer + SizeOfAttrLeafHeader(fInode));

		int TotalEntries = header->Count();

		delete header;

		uint16 curOffset = 1;

		for (int j = 0; j < TotalEntries; j++) {
			if (curOffset > fLastEntryOffset) {
				uint32 offset = B_BENDIAN_TO_HOST_INT16(entry->nameidx);
				TRACE("offset:(%" B_PRIu16 ")\n", offset);
				fLastEntryOffset = curOffset;

				// First check if its local or remote value
				if (entry->flags & XFS_ATTR_LOCAL) {
					AttrLeafNameLocal* local  = (AttrLeafNameLocal*)(fCurLeafBuffer + offset);
					memcpy(name, local->nameval, local->namelen);
					name[local->namelen] = '\0';
					*nameLength = local->namelen + 1;
					TRACE("Entry found name : %s, namelength : %ld", name, *nameLength);
					return B_OK;
				} else {
					AttrLeafNameRemote* remote  = (AttrLeafNameRemote*)(fCurLeafBuffer + offset);
					memcpy(name, remote->name, remote->namelen);
					name[remote->namelen] = '\0';
					*nameLength = remote->namelen + 1;
					TRACE("Entry found name : %s, namelength : %ld", name, *nameLength);
					return B_OK;
				}
			}
			curOffset++;
			entry = (AttrLeafEntry*)((char*)entry + sizeof(AttrLeafEntry));
		}
		Nentry = (NodeEntry*)((char*)Nentry + sizeof(NodeEntry));
		delete fCurLeafBuffer;
		fCurLeafBuffer = NULL;
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
NodeAttribute::Lookup(const char* name, size_t* nameLength)
{
	TRACE("NodeAttribute::Lookup\n");
	uint32 hashValueOfRequest = hashfunction(name, *nameLength);
	TRACE("Hashval:(%" B_PRIu32 ")\n", hashValueOfRequest);

	// first we need to find leaf block which might contain our entry
	NodeHeader* node = CreateNodeHeader(fInode, fNodeBuffer);
	NodeEntry* Nentry = (NodeEntry*)(fNodeBuffer + SizeOfNodeHeader(fInode));

	int TotalNodeEntries = node->Count();
	int left = 0;
	int mid;
	int right = TotalNodeEntries - 1;

	delete node;

	// LowerBound
	while (left < right) {
		mid = (left+right)/2;
		uint32 hashval = B_BENDIAN_TO_HOST_INT32(Nentry[mid].hashval);
		if (hashval >= hashValueOfRequest) {
			right = mid;
			continue;
		}
		if (hashval < hashValueOfRequest) {
			left = mid+1;
		}
	}
	TRACE("left:(%" B_PRId32 "), right:(%" B_PRId32 ")\n", left, right);

	// We found our potential leaf block, now read leaf buffer

	// First see the leaf block from NodeEntry and logical block offset
	uint32 LogicalBlock = B_BENDIAN_TO_HOST_INT32(Nentry[left].before);
	// Now calculate File system Block of This logical block
	xfs_fsblock_t block = LogicalToFileSystemBlock(LogicalBlock);
	FillBuffer(fLeafBuffer, block);

	AttrLeafHeader* header  = CreateAttrLeafHeader(fInode,fLeafBuffer);
	AttrLeafEntry* entry = (AttrLeafEntry*)(fLeafBuffer + SizeOfAttrLeafHeader(fInode));

	int numberOfLeafEntries = header->Count();
	left = 0;
	right = numberOfLeafEntries - 1;

	delete header;

	// LowerBound
	while (left < right) {
		mid = (left+right)/2;
		uint32 hashval = B_BENDIAN_TO_HOST_INT32(entry[mid].hashval);
		if (hashval >= hashValueOfRequest) {
			right = mid;
			continue;
		}
		if (hashval < hashValueOfRequest) {
			left = mid+1;
		}
	}
	TRACE("left:(%" B_PRId32 "), right:(%" B_PRId32 ")\n", left, right);

	while (B_BENDIAN_TO_HOST_INT32(entry[left].hashval)
			== hashValueOfRequest) {

		uint32 offset = B_BENDIAN_TO_HOST_INT16(entry[left].nameidx);
		TRACE("offset:(%" B_PRIu16 ")\n", offset);
		int status;

		// First check if its local or remote value
		if (entry[left].flags & XFS_ATTR_LOCAL) {
			AttrLeafNameLocal* local  = (AttrLeafNameLocal*)(fLeafBuffer + offset);
			char* PtrToOffset = (char*)local + sizeof(uint8) + sizeof(uint16);
			status = strncmp(name, PtrToOffset, *nameLength);
			if (status == 0) {
				fLocalEntry = local;
				fRemoteEntry = NULL;
				return B_OK;
			}
		} else {
			AttrLeafNameRemote* remote  = (AttrLeafNameRemote*)(fLeafBuffer + offset);
			char* PtrToOffset = (char*)remote + sizeof(uint8) + 2 * sizeof(uint32);
			status = strncmp(name, PtrToOffset, *nameLength);
			if (status == 0) {
				fRemoteEntry = remote;
				fLocalEntry = NULL;
				return B_OK;
			}
		}
		left++;
	}

	return B_ENTRY_NOT_FOUND;
}