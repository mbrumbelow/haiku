/*
 * Copyright 2015-2016, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck IV <kallisti5@unixzen.com>
 */
#ifndef _HASH_ALGORITHM_H
#define _HASH_ALGORITHM_H


#include <String.h>
#include <SupportDefs.h>


namespace BPrivate {

class HashAlgorithm {
public:
							HashAlgorithm() {}
	virtual					~HashAlgorithm() {}

	virtual status_t		Update(const void* input, size_t size) { return B_ERROR; }
	virtual status_t		Finish(uint8* digest) { return B_ERROR; }

	virtual BString			Name() { return NULL; }
	virtual size_t			MaxBuffer() { return 0; }
	virtual size_t			DigestLength() { return 0; }
};

} /* BPrivate */


#endif /* _HASH_ALGORITHM_H */
