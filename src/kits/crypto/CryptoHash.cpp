/*
 * Copyright 2015-2016, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV <kallisti5@unixzen.com>
 */

#include <DataIO.h>
#include <Debug.h>
#include <CryptoHash.h>
#include <String.h>

#include <new>

// Our supported algorithms
#include "Blake2.h"
#include "MD4.h"
#include "MD5.h"
#include "SHA256.h"


#undef TRACE

// #define TRACE_CRYPTO
#ifdef TRACE_CRYPTO
#   define TRACE(x...) _sPrintf("BCryptoHash: " x)
#else
#   define TRACE(x...) ;
#endif

#define ERROR(x...) _sPrintf("BCryptoHash: " x)


BCryptoHash::BCryptoHash(algorithm_type algorithm)
	:
	fAlgorithm(NULL),
	fPosition(0)
{
	switch (algorithm) {
		case B_HASH_BLAKE2:
			fAlgorithm = new BlakeAlgorithm();
			break;
		case B_HASH_MD4:
			fAlgorithm = new MD4Algorithm();
			break;
		case B_HASH_MD5:
			fAlgorithm = new MD5Algorithm();
			break;
		case B_HASH_SHA256:
			fAlgorithm = new SHA256Algorithm();
			break;
		case B_HASH_SHA512:
		default:
			ERROR("algorithm not yet implemented!\n");
	}
}


BCryptoHash::~BCryptoHash()
{
	if (fAlgorithm != NULL)
		delete fAlgorithm;
}


BString
BCryptoHash::Name()
{
	if (fAlgorithm != NULL)
		return fAlgorithm->Name();

	return "UNAVAILABLE";
}


status_t
BCryptoHash::AddData(const void* input, size_t length)
{
	if (input == NULL || length == 0)
		return B_BAD_VALUE;

	size_t bufferLength = fAlgorithm->MaxBuffer();
	size_t pos = 0;

	while (pos < length) {
		char buffer[bufferLength];
		if ((length - pos) < bufferLength)
			bufferLength = length - pos;
		memcpy(buffer, (char*)input + pos, bufferLength);
		fAlgorithm->Update(buffer, bufferLength);
		pos += bufferLength;
	}

	fPosition += length;
	return B_OK;
}


status_t
BCryptoHash::AddData(BString* input)
{
	if (input == NULL)
		return B_BAD_VALUE;
	return AddData((void*)input->String(), input->Length());
}


status_t
BCryptoHash::AddData(BFile* file)
{
	if (file == NULL || file->InitCheck() != B_OK)
		return B_BAD_VALUE;

	if (!file->IsReadable())
		return B_PERMISSION_DENIED;

	size_t bufferLength = fAlgorithm->MaxBuffer();
	char buffer[bufferLength];
	off_t length;

	file->Seek(0, SEEK_SET);
	file->GetSize(&length);
	while (file->Position() < length) {
		ssize_t len = file->Read(&buffer, bufferLength);
		fAlgorithm->Update(buffer, len);
	}

	fPosition += length;
	return B_OK;
}


status_t
BCryptoHash::Flush()
{
	fPosition = 0;

	if (fAlgorithm != NULL)
		fAlgorithm->Flush();

	return B_OK;
}


size_t
BCryptoHash::Position()
{
	return fPosition;
}


status_t
BCryptoHash::Result(BString* hash)
{
	if (hash != NULL) {
		size_t digestLength = fAlgorithm->DigestLength();
		uint8 result[digestLength];
		memset(result, 0, digestLength);
		fAlgorithm->Finish(result);

		for (size_t j = 0; j < digestLength; j++) {
			char hex[2];
			sprintf(hex, "%02x", result[j]);
			hash->Append(hex, 2);
		}
		return B_OK;
	}

	return B_BAD_VALUE;
}
