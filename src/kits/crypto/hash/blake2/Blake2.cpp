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

#include "Blake2.h"

// Theirs
#include "blake2.h"


BlakeAlgorithm::BlakeAlgorithm()
	: HashAlgorithm()
{
	blake2b_init(fState, BLAKE2B_OUTBYTES);
}


BlakeAlgorithm::~BlakeAlgorithm()
{
}


status_t
BlakeAlgorithm::Update(const void* input, size_t size)
{
	if (size > MaxBuffer())
		return B_BAD_VALUE;

	if (input == NULL)
		return B_BAD_VALUE;

	blake2b_update(fState, (uint8_t*)input, size);

	return B_OK;
}


status_t
BlakeAlgorithm::Finish(uint8* digest)
{
	if (digest == NULL)
		return B_BAD_VALUE;

	blake2b_final(fState, digest, DigestLength());
	return B_OK;
}
