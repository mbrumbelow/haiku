/*
 * Copyright 2015-2016, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck IV <kallisti5@unixzen.com>
 */
#ifndef _SHA256_ALGORITHM_H
#define _SHA256_ALGORITHM_H


#include <String.h>
#include <SupportDefs.h>

#include "HashAlgorithm.h"

#include "sha256.h"


using namespace BPrivate;


class SHA256Algorithm : public HashAlgorithm {
public:
							SHA256Algorithm();
							~SHA256Algorithm();

			status_t		Update(const void* input, size_t size);
			status_t		Finish(uint8* digest);

			BString			Name() { return "SHA256"; }
			size_t			MaxBuffer() { return 32768; }
			size_t			DigestLength() { return SHA256_BLOCK_SIZE; }

			void			Flush();
private:
			SHA256_CTX		fState;
};


#endif /* _MD5_ALGORITHM_H */

