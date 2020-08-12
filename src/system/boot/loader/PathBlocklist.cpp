/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <boot/PathBlocklist.h>

#include <stdlib.h>

#include <algorithm>


// #pragma mark - BlocklistedPath


BlocklistedPath::BlocklistedPath()
	:
	fPath(NULL),
	fLength(0),
	fCapacity(0)
{
}


BlocklistedPath::~BlocklistedPath()
{
	free(fPath);
}


bool
BlocklistedPath::SetTo(const char* path)
{
	size_t length = strlen(path);
	if (length > 0 && path[length - 1] == '/')
		length--;

	if (!_Resize(length, false))
		return false;

	if (length > 0) {
		memcpy(fPath, path, length);
		fPath[length] = '\0';
	}

	return true;
}


bool
BlocklistedPath::Append(const char* component)
{
	size_t componentLength = strlen(component);
	if (componentLength > 0 && component[componentLength - 1] == '/')
		componentLength--;
	if (componentLength == 0)
		return true;

	size_t oldLength = fLength;
	size_t length = (fLength > 0 ? fLength + 1 : 0) + componentLength;
	if (!_Resize(length, true))
		return false;

	if (oldLength > 0)
		fPath[oldLength++] = '/';
	memcpy(fPath + oldLength, component, componentLength);
	return true;
}


bool
BlocklistedPath::_Resize(size_t length, bool keepData)
{
	if (length == 0) {
		free(fPath);
		fPath = NULL;
		fLength = 0;
		fCapacity = 0;
		return true;
	}

	if (length < fCapacity) {
		fPath[length] = '\0';
		fLength = length;
		return true;
	}

	size_t capacity = std::max(length + 1, 2 * fCapacity);
	capacity = std::max(capacity, size_t(32));

	char* path;
	if (fLength > 0 && keepData) {
		path = (char*)realloc(fPath, capacity);
		if (path == NULL)
			return false;
	} else {
		path = (char*)malloc(capacity);
		if (path == NULL)
			return false;
		free(fPath);
	}

	fPath = path;
	fPath[length] = '\0';
	fLength = length;
	fCapacity = capacity;
	return true;
}


// #pragma mark - PathBlocklist


PathBlocklist::PathBlocklist()
{
}


PathBlocklist::~PathBlocklist()
{
	MakeEmpty();
}


bool
PathBlocklist::Add(const char* path)
{
	BlocklistedPath* blocklistedPath = _FindPath(path);
	if (blocklistedPath != NULL)
		return true;

	blocklistedPath = new(std::nothrow) BlocklistedPath;
	if (blocklistedPath == NULL || !blocklistedPath->SetTo(path)) {
		delete blocklistedPath;
		return false;
	}

	fPaths.Add(blocklistedPath);
	return true;
}


void
PathBlocklist::Remove(const char* path)
{
	BlocklistedPath* blocklistedPath = _FindPath(path);
	if (blocklistedPath != NULL) {
		fPaths.Remove(blocklistedPath);
		delete blocklistedPath;
	}
}


bool
PathBlocklist::Contains(const char* path) const
{
	return _FindPath(path) != NULL;
}


void
PathBlocklist::MakeEmpty()
{
	while (BlocklistedPath* blocklistedPath = fPaths.RemoveHead())
		delete blocklistedPath;
}


BlocklistedPath*
PathBlocklist::_FindPath(const char* path) const
{
	for (PathList::Iterator it = fPaths.GetIterator(); it.HasNext();) {
		BlocklistedPath* blocklistedPath = it.Next();
		if (*blocklistedPath == path)
			return blocklistedPath;
	}

	return NULL;
}
