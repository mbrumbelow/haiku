/*
 * Copyright 2010-2017 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include <HttpResult.h>
#include <Debug.h>


using std::ostream;


BHTTPResult::BHTTPResult(const BURL& url)
	:
	fUrl(url),
	fHeaders(),
	fStatusCode(0)
{
}


BHTTPResult::BHTTPResult(BMessage* archive)
	:
	BURLResult(archive),
	fUrl(archive->FindString("http:url")),
	fHeaders(),
	fStatusCode(archive->FindInt32("http:statusCode"))
{
	fStatusString = archive->FindString("http:statusString");

	BMessage headers;
	archive->FindMessage("http:headers", &headers);
	fHeaders.PopulateFromArchive(&headers);
}


BHTTPResult::BHTTPResult(const BHTTPResult& other)
	:
	fUrl(other.fUrl),
	fHeaders(other.fHeaders),
	fStatusCode(other.fStatusCode),
	fStatusString(other.fStatusString)
{
}


BHTTPResult::~BHTTPResult()
{
}


// #pragma mark Result parameters modifications


void
BHTTPResult::SetUrl(const BURL& url)
{
	fUrl = url;
}


// #pragma mark Result parameters access


const BURL&
BHTTPResult::Url() const
{
	return fUrl;
}


BString
BHTTPResult::ContentType() const
{
	return Headers()["Content-Type"];
}


size_t
BHTTPResult::Length() const
{
	const char* length = Headers()["Content-Length"];
	if (length == NULL)
		return 0;
	return atoi(length);
}


const BHTTPHeaders&
BHTTPResult::Headers() const
{
	return fHeaders;
}


int32
BHTTPResult::StatusCode() const
{
	return fStatusCode;
}


const BString&
BHTTPResult::StatusText() const
{
	return fStatusString;
}


// #pragma mark Result tests


bool
BHTTPResult::HasHeaders() const
{
	return fHeaders.CountHeaders() > 0;
}


// #pragma mark Overloaded members


BHTTPResult&
BHTTPResult::operator=(const BHTTPResult& other)
{
	if (this == &other)
		return *this;

	fUrl = other.fUrl;
	fHeaders = other.fHeaders;
	fStatusCode = other.fStatusCode;
	fStatusString = other.fStatusString;

	return *this;
}


status_t
BHTTPResult::Archive(BMessage* target, bool deep) const
{
	status_t result = BURLResult::Archive(target, deep);
	if (result != B_OK)
		return result;

	target->AddString("http:url", fUrl);
	target->AddInt32("http:statusCode", fStatusCode);
	target->AddString("http:statusString", fStatusString);

	BMessage headers;
	fHeaders.Archive(&headers);
	target->AddMessage("http:headers", &headers);

	return B_OK;
}


/*static*/ BArchivable*
BHTTPResult::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "BHTTPResult"))
		return NULL;

	return new BHTTPResult(archive);
}
