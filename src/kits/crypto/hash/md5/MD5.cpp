/*
 * Copyright 2015-2016, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck IV <kallisti5@unixzen.com>
 */


#include <String.h>
#include <SupportDefs.h>
#include <stdio.h>
#include <stdlib.h>

#include "MD5.h"

// Theirs
#include "md5.h"


MD5Algorithm::MD5Algorithm()
	: HashAlgorithm()
{
	MD5_Init(&fState);
}


MD5Algorithm::~MD5Algorithm()
{
}


status_t
MD5Algorithm::Update(const void* input, size_t size)
{
	if (size > MaxBuffer())
		return B_BAD_VALUE;

	if (input == NULL)
		return B_BAD_VALUE;

	MD5_Update(&fState, input, size);

	return B_OK;
}


status_t
MD5Algorithm::Finish(uint8* digest)
{
	if (digest == NULL)
		return B_BAD_VALUE;

	MD5_Final(digest, &fState);
	return B_OK;
}


void
MD5Algorithm::Flush()
{
	memset(&fState, 0, sizeof(fState));
	MD5_Init(&fState);
}
