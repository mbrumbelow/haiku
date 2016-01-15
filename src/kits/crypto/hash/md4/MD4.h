/*
 * Copyright 2015-2016, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck IV <kallisti5@unixzen.com>
 */
#ifndef _MD4_ALGORITHM_H
#define _MD4_ALGORITHM_H


#include <String.h>
#include <SupportDefs.h>

#include "HashAlgorithm.h"

#include "md4.h"


using namespace BPrivate;


class MD4Algorithm : public HashAlgorithm {
public:
							MD4Algorithm();
							~MD4Algorithm();

			status_t		Update(const void* input, size_t size);
			status_t		Finish(uint8* digest);

			BString			Name() { return "MD4"; }
			size_t			MaxBuffer() { return 32768; }
			size_t			DigestLength() { return 16; }

			void			Flush();

private:
			MD4_CTX			fState;
};


#endif /* _MD4_ALGORITHM_H */

