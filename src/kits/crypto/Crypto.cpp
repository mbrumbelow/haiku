/*
 * Copyright 2015-2016, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV <kallisti5@unixzen.com>
 */


#include <Debug.h>
#include <Crypto.h>
#include <String.h>


#undef TRACE

#define TRACE_CRYPTO
#ifdef TRACE_CRYPTO
#   define TRACE(x...) _sPrintf("BCrypto: " x)
#else
#   define TRACE(x...) ;
#endif

#define ERROR(x...) _sPrintf("BCrypto: " x)


static void
rand_str(char *dest, size_t length)
{
	char charset[] = "0123456789abcdefghijklmnopqrstuvwxyz";

	while (length-- > 0) {
		size_t index = (double) rand() / RAND_MAX * (sizeof charset - 1);
		*dest++ = charset[index];
	}
	*dest = '\0';
}


BCrypto::BCrypto(cipher_type cipher)
	:
	fCipher(cipher)
{


}


BCrypto::~BCrypto()
{
	// Introduce some random data into the key/salt before delete
	for (int round = 0; round < 4; round++) {
		char randomKey[fKey.CountChars() + 1];
		rand_str(randomKey, fKey.CountChars());
		fKey.SetTo(randomKey);
	}
}
