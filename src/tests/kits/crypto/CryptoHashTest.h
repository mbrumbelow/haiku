/*
 * Copyright 2021-2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck IV, kallisti5@unixzen.com
 */
#ifndef __CRYPTOHASH_TEST
#define __CRYPTOHASH_TEST


#include <crypto/CryptoHash.h>

/** CppUnit support */
#include <TestCase.h>


class CryptoHashTest : public BTestCase {
public:
	CryptoHashTest(std::string name = "");
	~CryptoHashTest();

	// cppunit suite function prototype
	static CppUnit::Test *Suite();

	// actual unit tests
	void Blake2HashTest();
};


#endif /* __CRYPTOHASH_TEST */
