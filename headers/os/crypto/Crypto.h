/*
 * Copyright 2015-2016, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck IV <kallisti5@unixzen.com>
 */
#ifndef _CRYPTO_H
#define _CRYPTO_H


#include <BeBuild.h>
#include <OS.h>
#include <String.h>
#include <SupportDefs.h>

#include <stddef.h>


enum cipher_type {
	B_CIPHER_AES = 0,
	B_CIPHER_BLOWFISH,
	B_CIPHER_SERPENT
};


class BCrypto {
public:
							BCrypto(cipher_type cipher);
							~BCrypto();

			status_t		SetKey(BString* key);

// TODO: BDataIO for data stream to crypt / decrypt

private:
			cipher_type		fCipher;

			BString			fKey;
};


#endif /* _CRYPTO_H */
