/*
 * Copyright 2022, Raghav Sharma, raghavself28@gmail.com
 * Distributed under the terms of the MIT License.
 */


#include "LeafAttribute.h"

#include "VerifyHeader.h"


LeafAttribute::LeafAttribute(Inode* inode)
	:
	fInode(inode),
	fName(NULL)
{
	fLastEntryOffset = 0;
}


LeafAttribute::LeafAttribute(Inode* inode, const char* name)
	:
	fInode(inode),
	fName(name)
{
	fLastEntryOffset = 0;
}


LeafAttribute::~LeafAttribute()
{
	delete fMap;
	delete fLeafBuffer;
}


status_t
LeafAttribute::Init()
{
	status_t status;

	status = FillMapEntry();

	if (status != B_OK)
		return status;

	status = FillLeafBuffer();

	if (status != B_OK)
		return status;

	AttrLeafHeader* header  = CreateAttrLeafHeader(fInode,fLeafBuffer);
	if (header == NULL)
		return B_NO_MEMORY;

	if (!VerifyHeader<AttrLeafHeader>(header, fLeafBuffer, fInode, 0, fMap, ATTR_LEAF)) {
		ERROR("Invalid data header");
		delete header;
		return B_BAD_VALUE;
	}
	delete header;

	return B_OK;
}


status_t
LeafAttribute::FillMapEntry()
{
	fMap = new(std::nothrow) ExtentMapEntry;
	if (fMap == NULL)
		return B_NO_MEMORY;

	void* AttributeFork = DIR_AFORK_PTR(fInode->Buffer(),
		fInode->CoreInodeSize(), fInode->ForkOffset());

	uint64* pointerToMap = (uint64*)((char*)AttributeFork);
	uint64 firstHalf = pointerToMap[0];
	uint64 secondHalf = pointerToMap[1];
		//dividing the 128 bits into 2 parts.

	firstHalf = B_BENDIAN_TO_HOST_INT64(firstHalf);
	secondHalf = B_BENDIAN_TO_HOST_INT64(secondHalf);
	fMap->br_state = firstHalf >> 63;
	fMap->br_startoff = (firstHalf & MASK(63)) >> 9;
	fMap->br_startblock = ((firstHalf & MASK(9)) << 43) | (secondHalf >> 21);
	fMap->br_blockcount = secondHalf & MASK(21);
	TRACE("Extent::Init: startoff:(%" B_PRIu64 "), startblock:(%" B_PRIu64 "),"
		"blockcount:(%" B_PRIu64 "),state:(%" B_PRIu8 ")\n", fMap->br_startoff,
		fMap->br_startblock, fMap->br_blockcount, fMap->br_state);

	return B_OK;
}


status_t
LeafAttribute::FillLeafBuffer()
{
	if (fMap->br_state != 0)
		return B_BAD_VALUE;

	int len = fInode->DirBlockSize();
	fLeafBuffer = new(std::nothrow) char[len];
	if (fLeafBuffer == NULL)
		return B_NO_MEMORY;

	xfs_daddr_t readPos =
		fInode->FileSystemBlockToAddr(fMap->br_startblock);

	if (read_pos(fInode->GetVolume()->Device(), readPos, fLeafBuffer, len)
		!= len) {
		ERROR("Extent::FillBlockBuffer(): IO Error");
		return B_IO_ERROR;
	}

	return B_OK;
}


status_t
LeafAttribute::Stat(struct stat& stat)
{
	TRACE("LeafAttribute::Stat\n");

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
LeafAttribute::Read(attr_cookie* cookie, off_t pos, uint8* buffer, size_t* length)
{
	TRACE("LeafAttribute::Read\n");

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
LeafAttribute::GetNext(char* name, size_t* nameLength)
{
	TRACE("LeafAttribute::GetNext\n");

	AttrLeafHeader* header  = CreateAttrLeafHeader(fInode,fLeafBuffer);
	AttrLeafEntry* entry = (AttrLeafEntry*)(fLeafBuffer + SizeOfAttrLeafHeader(fInode));

	int TotalEntries = header->Count();
	uint16 curOffset = 1;

	delete header;

	for (int i = 0; i < TotalEntries; i++) {
		if (curOffset > fLastEntryOffset) {
			uint32 offset = B_BENDIAN_TO_HOST_INT16(entry->nameidx);
			TRACE("offset:(%" B_PRIu16 ")\n", offset);
			fLastEntryOffset = curOffset;

			// First check if its local or remote value
			if (entry->flags & XFS_ATTR_LOCAL) {
				AttrLeafNameLocal* local  = (AttrLeafNameLocal*)(fLeafBuffer + offset);
				memcpy(name, local->nameval, local->namelen);
				name[local->namelen] = '\0';
				*nameLength = local->namelen + 1;
				TRACE("Entry found name : %s, namelength : %ld", name, *nameLength);
				return B_OK;
			} else {
				AttrLeafNameRemote* remote  = (AttrLeafNameRemote*)(fLeafBuffer + offset);
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

	return B_ENTRY_NOT_FOUND;

}


status_t
LeafAttribute::Lookup(const char* name, size_t* nameLength)
{
	TRACE("LeafAttribute::Lookup\n");
	uint32 hashValueOfRequest = hashfunction(name, *nameLength);
	TRACE("Hashval:(%" B_PRIu32 ")\n", hashValueOfRequest);

	AttrLeafHeader* header  = CreateAttrLeafHeader(fInode,fLeafBuffer);
	AttrLeafEntry* entry = (AttrLeafEntry*)(fLeafBuffer + SizeOfAttrLeafHeader(fInode));

	int numberOfLeafEntries = header->Count();
	int left = 0;
	int mid;
	int right = numberOfLeafEntries - 1;

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


AttrLeafHeader::~AttrLeafHeader()
{
}


/*
	First see which type of directory we reading then
	return magic number as per Inode Version.
*/
uint32
AttrLeafHeader::ExpectedMagic(int8 WhichDirectory, Inode* inode)
{
	if (WhichDirectory == ATTR_LEAF) {
		if (inode->Version() == 1 || inode->Version() == 2)
			return XFS_ATTR_LEAF_MAGIC;
		else
			return XFS_ATTR3_LEAF_MAGIC;
	} else {
		// currently we don't support other directories;
		return B_BAD_VALUE;
	}
}


uint32
AttrLeafHeader::CRCOffset()
{
	return ATTR_LEAF_CRC_OFF - ATTR_LEAF_V5_VPTR_OFF;
}


void
AttrLeafHeaderV4::SwapEndian()
{
	info.forw	=	B_BENDIAN_TO_HOST_INT32(info.forw);
	info.back	=	B_BENDIAN_TO_HOST_INT32(info.back);
	info.magic	= 	B_BENDIAN_TO_HOST_INT16(info.magic);
	info.pad	=	B_BENDIAN_TO_HOST_INT16(info.pad);
	count		=	B_BENDIAN_TO_HOST_INT16(count);
	usedbytes	=	B_BENDIAN_TO_HOST_INT16(usedbytes);
	firstused	=	B_BENDIAN_TO_HOST_INT16(firstused);
}


AttrLeafHeaderV4::AttrLeafHeaderV4(const char* buffer)
{
	uint32 offset = 0;

	info = *(BlockInfo*)(buffer + offset);
	offset += sizeof(BlockInfo);

	count = *(uint16*)(buffer + offset);
	offset += sizeof(uint16);

	usedbytes = *(uint16*)(buffer + offset);
	offset += sizeof(uint16);

	firstused = *(uint16*)(buffer + offset);
	offset += sizeof(uint16);

	holes = *(uint8*)(buffer + offset);
	offset += sizeof(uint8);

	pad1 = *(uint8*)(buffer + offset);
	offset += sizeof(uint8);

	memcpy(freemap, buffer + offset, XFS_ATTR_LEAF_MAPSIZE * sizeof(AttrLeafMap));

	SwapEndian();
}


AttrLeafHeaderV4::~AttrLeafHeaderV4()
{
}


uint16
AttrLeafHeaderV4::Magic()
{
	return info.magic;
}


uint64
AttrLeafHeaderV4::Blockno()
{
	return B_BAD_VALUE;
}


uint64
AttrLeafHeaderV4::Owner()
{
	return B_BAD_VALUE;
}


uuid_t*
AttrLeafHeaderV4::Uuid()
{
	return NULL;
}


uint16
AttrLeafHeaderV4::Count()
{
	return count;
}


void
AttrLeafHeaderV5::SwapEndian()
{
	info.forw	=	B_BENDIAN_TO_HOST_INT32(info.forw);
	info.back	=	B_BENDIAN_TO_HOST_INT32(info.back);
	info.magic	= 	B_BENDIAN_TO_HOST_INT16(info.magic);
	info.pad	=	B_BENDIAN_TO_HOST_INT16(info.pad);
	info.blkno	=	B_BENDIAN_TO_HOST_INT64(info.blkno);
	info.lsn	=	B_BENDIAN_TO_HOST_INT64(info.lsn);
	info.owner	=	B_BENDIAN_TO_HOST_INT64(info.owner);
	count		=	B_BENDIAN_TO_HOST_INT16(count);
	usedbytes	=	B_BENDIAN_TO_HOST_INT16(usedbytes);
	firstused	=	B_BENDIAN_TO_HOST_INT16(firstused);
	pad2		=	B_BENDIAN_TO_HOST_INT32(pad2);
}


AttrLeafHeaderV5::AttrLeafHeaderV5(const char* buffer)
{
	uint32 offset = 0;

	info = *(BlockInfoV5*)(buffer + offset);
	offset += sizeof(BlockInfoV5);

	count = *(uint16*)(buffer + offset);
	offset += sizeof(uint16);

	usedbytes = *(uint16*)(buffer + offset);
	offset += sizeof(uint16);

	firstused = *(uint16*)(buffer + offset);
	offset += sizeof(uint16);

	holes = *(uint8*)(buffer + offset);
	offset += sizeof(uint8);

	pad1 = *(uint8*)(buffer + offset);
	offset += sizeof(uint8);

	memcpy(freemap, buffer + offset, XFS_ATTR_LEAF_MAPSIZE * sizeof(AttrLeafMap));
	offset += XFS_ATTR_LEAF_MAPSIZE * sizeof(AttrLeafMap);

	pad2 = *(uint32*)(buffer + offset);
	offset += sizeof(uint32);

	SwapEndian();
}


AttrLeafHeaderV5::~AttrLeafHeaderV5()
{
}


uint16
AttrLeafHeaderV5::Magic()
{
	return info.magic;
}


uint64
AttrLeafHeaderV5::Blockno()
{
	return info.blkno;
}


uint64
AttrLeafHeaderV5::Owner()
{
	return info.owner;
}


uuid_t*
AttrLeafHeaderV5::Uuid()
{
	return &info.uuid;
}


uint16
AttrLeafHeaderV5::Count()
{
	return count;
}


//Function to get V4 or V5 Attr leaf header instance
AttrLeafHeader*
CreateAttrLeafHeader(Inode* inode, const char* buffer)
{
	if (inode->Version() == 1 || inode->Version() == 2) {
		AttrLeafHeaderV4* header = new (std::nothrow) AttrLeafHeaderV4(buffer);
		return header;
	} else {
		AttrLeafHeaderV5* header = new (std::nothrow) AttrLeafHeaderV5(buffer);
		return header;
	}
}


/*
	This Function returns Actual size of leaf header
	in all forms of directory.
	Never use sizeof() operator because we now have
	vtable as well and it will give wrong results
*/
uint32
SizeOfAttrLeafHeader(Inode* inode)
{
	if (inode->Version() == 1 || inode->Version() == 2)
		return sizeof(AttrLeafHeaderV4) - ATTR_LEAF_V4_VPTR_OFF;
	else
		return sizeof(AttrLeafHeaderV5) - ATTR_LEAF_V5_VPTR_OFF;
}

