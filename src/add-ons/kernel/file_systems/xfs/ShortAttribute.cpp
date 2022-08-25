/*
 * Copyright 2022, Raghav Sharma, raghavself28@gmail.com
 * Distributed under the terms of the MIT License.
 */


#include "ShortAttribute.h"


ShortAttribute::ShortAttribute(Inode* inode)
	:
	fInode(inode),
	fName(NULL)
{
	fHeader = (AShortFormHeader*)(DIR_AFORK_PTR(fInode->Buffer(),
		fInode->CoreInodeSize(), fInode->ForkOffset()));

	fLastEntryOffset = 0;
}


ShortAttribute::ShortAttribute(Inode* inode, const char* name)
	:
	fInode(inode),
	fName(name)
{
	fHeader = (AShortFormHeader*)(DIR_AFORK_PTR(fInode->Buffer(),
		fInode->CoreInodeSize(), fInode->ForkOffset()));

	fLastEntryOffset = 0;
}


ShortAttribute::~ShortAttribute()
{
}


uint32
ShortAttribute::DataLength(AShortFormEntry* entry)
{
	return entry->namelen + entry->valuelen;
}


AShortFormEntry*
ShortAttribute::FirstEntry()
{
	return (AShortFormEntry*) ((char*) fHeader + sizeof(AShortFormHeader));
}


status_t
ShortAttribute::Stat(struct stat& stat)
{
	TRACE("Short Attribute : Stat\n");

	size_t namelength = strlen(fName);

	status_t status;

	// check if this attribute exists
	status = Lookup(fName, &namelength);

	if(status != B_OK)
		return status;

	// We have valid attribute entry to stat
	stat.st_type = B_XATTR_TYPE;
	stat.st_size = DataLength(fEntry);

	return B_OK;
}


status_t
ShortAttribute::Read(attr_cookie* cookie, off_t pos, uint8* buffer, size_t* length)
{
	TRACE("Short Attribute : Read\n");

	if(pos < 0)
		return B_BAD_VALUE;

	size_t namelength = strlen(fName);

	status_t status;

	status = Lookup(fName, &namelength);

	uint32 LengthToRead = 0;

	if (pos + *length > fEntry->valuelen)
		LengthToRead = fEntry->valuelen - pos;
	else
		LengthToRead = *length;

	char* PtrToOffset = (char*) fHeader + sizeof(AShortFormHeader)
		+ 3 * sizeof(uint8) + fEntry->namelen;

	memcpy(buffer, PtrToOffset, LengthToRead);

	*length = LengthToRead;

	return B_OK;
}


status_t
ShortAttribute::GetNext(char* name, size_t* nameLength)
{
	TRACE("Short Attribute : GetNext\n");

	AShortFormEntry* entry = FirstEntry();
	uint16 curOffset = 1;
	for (int i = 0; i < fHeader->count; i++) {
		if (curOffset > fLastEntryOffset) {

			fLastEntryOffset = curOffset;

			char* PtrToOffset = (char*)entry + 3 * sizeof(uint8);

			memcpy(name, PtrToOffset, entry->namelen);
			name[entry->namelen] = '\0';
			*nameLength = entry->namelen + 1;
			TRACE("Entry found name : %s, namelength : %ln", name, *nameLength);
			return B_OK;
		}
		entry = (AShortFormEntry*)((char*)entry + 3 * sizeof(uint8) + DataLength(entry));
		curOffset += 3 * sizeof(uint8) + DataLength(entry);
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
ShortAttribute::Lookup(const char* name, size_t* nameLength)
{
	TRACE("Short Attribute : Lookup\n");

	AShortFormEntry* entry = FirstEntry();

	int status;

	for (int i = 0; i < fHeader->count; i++) {
		char* PtrToOffset = (char*)entry + 3 * sizeof(uint8);
		status = strncmp(name, PtrToOffset, *nameLength);
		if (status == 0) {
			fEntry = entry;
			return B_OK;
		}
		entry = (AShortFormEntry*)((char*)entry + 3 * sizeof(uint8) + DataLength(entry));
	}

	return B_ENTRY_NOT_FOUND;
}
