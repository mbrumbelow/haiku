/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 */


#include <ctype.h>
#include <string.h>
#include <new>

#include <String.h>
#include <HttpHeaders.h>


// #pragma mark -- BHTTPHeader


BHTTPHeader::BHTTPHeader()
	:
	fName(),
	fValue(),
	fRawHeader(),
	fRawHeaderValid(true)
{
}


BHTTPHeader::BHTTPHeader(const char* string)
	:
	fRawHeaderValid(true)
{
	SetHeader(string);
}


BHTTPHeader::BHTTPHeader(const char* name, const char* value)
	:
	fRawHeaderValid(false)
{
	SetName(name);
	SetValue(value);
}


BHTTPHeader::BHTTPHeader(const BHTTPHeader& copy)
	:
	fName(copy.fName),
	fValue(copy.fValue),
	fRawHeaderValid(false)
{
}


void
BHTTPHeader::SetName(const char* name)
{
	fRawHeaderValid = false;
	fName = name;
	fName.Trim().CapitalizeEachWord();
}


void
BHTTPHeader::SetValue(const char* value)
{
	fRawHeaderValid = false;
	fValue = value;
	fValue.Trim();
}


bool
BHTTPHeader::SetHeader(const char* string)
{
	fRawHeaderValid = false;
	fName.Truncate(0);
	fValue.Truncate(0);

	const char* separator = strchr(string, ':');

	if (separator == NULL)
		return false;

	fName.SetTo(string, separator - string);
	fName.Trim().CapitalizeEachWord();
	SetValue(separator + 1);
	return true;
}


const char*
BHTTPHeader::Name() const
{
	return fName.String();
}


const char*
BHTTPHeader::Value() const
{
	return fValue.String();
}


const char*
BHTTPHeader::Header() const
{
	if (!fRawHeaderValid) {
		fRawHeaderValid = true;

		fRawHeader.Truncate(0);
		fRawHeader << fName << ": " << fValue;
	}

	return fRawHeader.String();
}


bool
BHTTPHeader::NameIs(const char* name) const
{
	return fName == BString(name).Trim().CapitalizeEachWord();
}


BHTTPHeader&
BHTTPHeader::operator=(const BHTTPHeader& other)
{
	fName = other.fName;
	fValue = other.fValue;
	fRawHeaderValid = false;

	return *this;
}


// #pragma mark -- BHTTPHeaders


BHTTPHeaders::BHTTPHeaders()
	:
	fHeaderList()
{
}


BHTTPHeaders::BHTTPHeaders(const BHTTPHeaders& other)
	:
	fHeaderList()
{
	*this = other;
}


BHTTPHeaders::~BHTTPHeaders()
{
	_EraseData();
}


// #pragma mark Header access


const char*
BHTTPHeaders::HeaderValue(const char* name) const
{
	for (int32 i = 0; i < fHeaderList.CountItems(); i++) {
		BHTTPHeader* header
			= reinterpret_cast<BHTTPHeader*>(fHeaderList.ItemAtFast(i));

		if (header->NameIs(name))
			return header->Value();
	}

	return NULL;
}


BHTTPHeader&
BHTTPHeaders::HeaderAt(int32 index) const
{
	//! Note: index _must_ be in-bounds
	BHTTPHeader* header
		= reinterpret_cast<BHTTPHeader*>(fHeaderList.ItemAtFast(index));

	return *header;
}


// #pragma mark Header count


int32
BHTTPHeaders::CountHeaders() const
{
	return fHeaderList.CountItems();
}


// #pragma Header tests


int32
BHTTPHeaders::HasHeader(const char* name) const
{
	for (int32 i = 0; i < fHeaderList.CountItems(); i++) {
		BHTTPHeader* header
			= reinterpret_cast<BHTTPHeader*>(fHeaderList.ItemAt(i));

		if (header->NameIs(name))
			return i;
	}

	return -1;
}


// #pragma mark Header add/replace


bool
BHTTPHeaders::AddHeader(const char* line)
{
	return _AddOrDeleteHeader(new(std::nothrow) BHTTPHeader(line));
}


bool
BHTTPHeaders::AddHeader(const char* name, const char* value)
{
	return _AddOrDeleteHeader(new(std::nothrow) BHTTPHeader(name, value));
}


bool
BHTTPHeaders::AddHeader(const char* name, int32 value)
{
	BString strValue;
	strValue << value;

	return AddHeader(name, strValue);
}


// #pragma mark Archiving


void
BHTTPHeaders::PopulateFromArchive(BMessage* archive)
{
	Clear();

	int32 index = 0;
	char* nameFound;
	for (;;) {
		if (archive->GetInfo(B_STRING_TYPE, index, &nameFound, NULL) != B_OK)
			return;

		BString value = archive->FindString(nameFound);
		AddHeader(nameFound, value);

		index++;
	}
}


void
BHTTPHeaders::Archive(BMessage* message) const
{
	int32 count = CountHeaders();

	for (int32 i = 0; i < count; i++) {
		BHTTPHeader& header = HeaderAt(i);
		message->AddString(header.Name(), header.Value());
	}
}


// #pragma mark Header deletion


void
BHTTPHeaders::Clear()
{
	_EraseData();
	fHeaderList.MakeEmpty();
}


// #pragma mark Overloaded operators


BHTTPHeaders&
BHTTPHeaders::operator=(const BHTTPHeaders& other)
{
	if (&other == this)
		return *this;

	Clear();

	for (int32 i = 0; i < other.CountHeaders(); i++)
		AddHeader(other.HeaderAt(i).Name(), other.HeaderAt(i).Value());

	return *this;
}


BHTTPHeader&
BHTTPHeaders::operator[](int32 index) const
{
	//! Note: Index _must_ be in-bounds
	BHTTPHeader* header
		= reinterpret_cast<BHTTPHeader*>(fHeaderList.ItemAtFast(index));

	return *header;
}


const char*
BHTTPHeaders::operator[](const char* name) const
{
	return HeaderValue(name);
}


void
BHTTPHeaders::_EraseData()
{
	// Free allocated data;
	for (int32 i = 0; i < fHeaderList.CountItems(); i++) {
		BHTTPHeader* header
			= reinterpret_cast<BHTTPHeader*>(fHeaderList.ItemAtFast(i));

		delete header;
	}
}


bool
BHTTPHeaders::_AddOrDeleteHeader(BHTTPHeader* header)
{
	if (header != NULL) {
		if (fHeaderList.AddItem(header))
			return true;
		delete header;
	}
	return false;
}
