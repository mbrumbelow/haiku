/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2014 Haiku, inc.
 *
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>

#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <pthread.h>


namespace BPrivate {


class SSL {
public:
	SSL()
	{
		int64 seed = find_thread(NULL) ^ system_time();
		RAND_seed(&seed, sizeof(seed));
	}

	~SSL()
	{
	}
};


static SSL sSSL;


}	// namespace BPrivate
