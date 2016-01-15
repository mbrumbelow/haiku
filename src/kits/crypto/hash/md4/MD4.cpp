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

#include "MD4.h"

// Theirs
#include "md4.h"


MD4Algorithm::MD4Algorithm()
	: HashAlgorithm()
{
	MD4_Init(&fState);
}


MD4Algorithm::~MD4Algorithm()
{
}


status_t
MD4Algorithm::Update(const void* input, size_t size)
{
	if (size > MaxBuffer())
		return B_BAD_VALUE;

	if (input == NULL)
		return B_BAD_VALUE;

	MD4_Update(&fState, input, size);

	return B_OK;
}


status_t
MD4Algorithm::Finish(uint8* digest)
{
	if (digest == NULL)
		return B_BAD_VALUE;

	MD4_Final(digest, &fState);
	return B_OK;
}


void
MD4Algorithm::Flush()
{
	memset(&fState, 0, sizeof(fState));
	MD4_Init(&fState);
}
