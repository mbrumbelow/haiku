/*
 * Copyright 2013-2017 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include <UrlResult.h>


BURLResult::BURLResult()
	:
	BArchivable(),
	fContentType(),
	fLength(0)
{
}


BURLResult::BURLResult(BMessage* archive)
	:
	BArchivable(archive)
{
	fContentType = archive->FindString("ContentType");
	fLength = archive->FindInt32("Length");
}


BURLResult::~BURLResult()
{
}


status_t
BURLResult::Archive(BMessage* archive, bool deep) const
{
	status_t result = BArchivable::Archive(archive, deep);

	if (result != B_OK)
		return result;

	archive->AddString("ContentType", fContentType);
	archive->AddInt32("Length", fLength);

	return B_OK;
}


void
BURLResult::SetContentType(BString contentType)
{
	fContentType = contentType;
}


void
BURLResult::SetLength(size_t length)
{
	fLength = length;
}


BString
BURLResult::ContentType() const
{
	return fContentType;
}


size_t
BURLResult::Length() const
{
	return fLength;
}


/*static*/ BArchivable*
BURLResult::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "BURLResult"))
		return NULL;
	return new BURLResult(archive);
}
