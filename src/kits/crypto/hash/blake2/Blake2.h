/*
 * Copyright 2015-2016, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck IV <kallisti5@unixzen.com>
 */
#ifndef _BLAKE_ALGORITHM_H
#define _BLAKE_ALGORITHM_H


#include <String.h>
#include <SupportDefs.h>

#include "HashAlgorithm.h"

#include "blake2.h"


using namespace BPrivate;


class BlakeAlgorithm : public HashAlgorithm {
public:
							BlakeAlgorithm();
							~BlakeAlgorithm();

			status_t		Update(const void* input, size_t size);
			status_t		Finish(uint8* digest);

			BString			Name() { return "blake2b"; }
			size_t			MaxBuffer() { return 32768; }
			size_t			DigestLength() { return BLAKE2B_OUTBYTES; }

private:
			blake2b_state	fState[1];
};


#endif /* _BLAKE_ALGORITHM_H */

