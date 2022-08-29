/*
 * Copyright 2022, Raghav Sharma, raghavself28@gmail.com
 * Distributed under the terms of the MIT License.
 */


#include "Attribute.h"

#include "Utility.h"


Attribute::Attribute(Inode* inode)
	:
	fInode(inode),
	fName(NULL),
	fShortAttr(NULL),
	fLeafAttr(NULL)
{
}


Attribute::Attribute(Inode* inode, attr_cookie* cookie)
	:
	fInode(inode),
	fName(cookie->name),
	fShortAttr(NULL),
	fLeafAttr(NULL)
{
}


Attribute::~Attribute()
{
	delete fShortAttr;
	delete fLeafAttr;
}


status_t
Attribute::Init()
{
	if (fInode->AFormat() == XFS_DINODE_FMT_LOCAL) {
		TRACE("Attribute::Init: LOCAL\n");
		if(fName == NULL)
			fShortAttr = new(std::nothrow) ShortAttribute(fInode);
		else
			fShortAttr = new(std::nothrow) ShortAttribute(fInode, fName);
		if (fShortAttr == NULL)
			return B_NO_MEMORY;
		return B_OK;
	}
	if (fInode->AFormat() == XFS_DINODE_FMT_EXTENTS) {
		TRACE("Attribute::Init: EXTENTS\n");
		if (fName == NULL)
			fLeafAttr = new(std::nothrow) LeafAttribute(fInode);
		else
			fLeafAttr = new(std::nothrow) LeafAttribute(fInode, fName);
		if (fLeafAttr == NULL)
			return B_NO_MEMORY;

		status_t status = fLeafAttr->Init();

		if (status == B_OK)
			return status;

		delete fLeafAttr;
		fLeafAttr = NULL;

		// Currently node attributes are not supported return B_BAD_VALUE
		return B_BAD_VALUE;
	}

	return B_BAD_VALUE;
}


status_t
Attribute::CheckAccess(int openMode)
{
	return fInode->CheckPermissions(open_mode_to_access(openMode)
		| (openMode & O_TRUNC ? W_OK : 0));
}


status_t
Attribute::Open(const char* name, int openMode, attr_cookie** _cookie)
{
	TRACE("Attribute::Open\n");
	status_t status = CheckAccess(openMode);
	if (status < B_OK)
		return status;

	size_t length = strlen(name);
	status = Lookup(name, &length);
	if (status < B_OK)
		return status;

	attr_cookie* cookie = new(std::nothrow) attr_cookie;
	if (cookie == NULL)
		return B_NO_MEMORY;

	fName = name;

	// initialize the cookie
	strlcpy(cookie->name, fName, B_ATTR_NAME_LENGTH);
	cookie->open_mode = openMode;
	cookie->create = false;

	*_cookie = cookie;
	return B_OK;
}


status_t
Attribute::Stat(struct stat& stat)
{
	if (fShortAttr != NULL)
		return fShortAttr->Stat(stat);
	else if (fLeafAttr != NULL)
		return fLeafAttr->Stat(stat);
	else
		return B_BAD_VALUE;
}


status_t
Attribute::Read(attr_cookie* cookie, off_t pos, uint8* buffer, size_t* length)
{
	if (fShortAttr != NULL)
		return fShortAttr->Read(cookie, pos, buffer, length);
	else if (fLeafAttr != NULL)
		return fLeafAttr->Read(cookie, pos, buffer, length);
	else
		return B_BAD_VALUE;
}


status_t
Attribute::GetNext(char* name, size_t* nameLength)
{
	if (fShortAttr != NULL)
		return fShortAttr->GetNext(name, nameLength);
	else if (fLeafAttr != NULL)
		return fLeafAttr->GetNext(name, nameLength);
	else
		return B_BAD_VALUE;
}


status_t
Attribute::Lookup(const char* name, size_t* nameLength)
{
	if (fShortAttr != NULL)
		return fShortAttr->Lookup(name, nameLength);
	else if (fLeafAttr != NULL)
		return fLeafAttr->Lookup(name, nameLength);
	else
		return B_BAD_VALUE;
}