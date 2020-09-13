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

#include "SHA256.h"

// Theirs
#include "sha256.h"


SHA256Algorithm::SHA256Algorithm()
	: HashAlgorithm()
{
	sha256_init(&fState);
}


SHA256Algorithm::~SHA256Algorithm()
{
}


status_t
SHA256Algorithm::Update(const void* input, size_t size)
{
	if (size > MaxBuffer())
		return B_BAD_VALUE;

	if (input == NULL)
		return B_BAD_VALUE;

	sha256_update(&fState, (unsigned char*)input, size);

	return B_OK;
}


status_t
SHA256Algorithm::Finish(uint8* digest)
{
	if (digest == NULL)
		return B_BAD_VALUE;

	sha256_final(&fState, digest);
	return B_OK;
}


void
SHA256Algorithm::Flush()
{
	memset(&fState, 0, sizeof(fState));
	sha256_init(&fState);
}
