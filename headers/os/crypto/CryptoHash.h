/*
 * Copyright 2015-2016, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck IV <kallisti5@unixzen.com>
 */
#ifndef _CRYPTO_HASH_H
#define _CRYPTO_HASH_H


#include <BeBuild.h>
#include <DataIO.h>
#include <File.h>
#include <OS.h>
#include <String.h>
#include <SupportDefs.h>

#include <stddef.h>


namespace BPrivate {
	class HashAlgorithm;
};


enum algorithm_type {
	B_HASH_BLAKE2 = 0,
	B_HASH_MD5,
	B_HASH_SHA1,
	B_HASH_SHA256,
	B_HASH_SHA512
};


class BCryptoHash {
public:
							BCryptoHash(algorithm_type algorithm);
							~BCryptoHash();

			BString			Name();

			status_t		AddData(BString* string);
			status_t		AddData(const void* input, size_t length);
			status_t		AddData(BFile* file);
//			status_t		AddData(area_id id);

			status_t		Result(BString* hash);

			status_t		Reset();
			size_t			Position();

private:
			BPrivate::HashAlgorithm*	fAlgorithm;
			size_t			fPosition;
};


#endif /* _CRYPTO_HASH_H */
