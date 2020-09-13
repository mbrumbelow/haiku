/*
 * Copyright 2021-2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *	Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include <stdio.h>

#include <CryptoHash.h>
#include <OS.h>
#include <Handler.h>
#include <Looper.h>
#include <String.h>

#include <TestShell.h>
#include <TestUtils.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include "CryptoHashTest.h"


CryptoHashTest::CryptoHashTest(std::string name)
	:
	BTestCase(name)
{
}


CryptoHashTest::~CryptoHashTest()
{
}


/*
        @case 1                 checksum a BString and compare it to a known valid hash.
        @results                Validation of successful hashing.
*/
void CryptoHashTest::Blake2HashTest()
{
	BString knownHash("0389abc5ab1e8e170e95aff19d341ecbf88b83a12dd657291ec1254108ea97352c2ff5116902b9fe4021bfe5a6a4372b0f7c9fc2d7dd810c29f85511d1e04c59");
	BString test("Hello world!");
	BString hash;
	status_t result = B_OK;

	// Init BCryptoHash
	BCryptoHash blake2(B_HASH_BLAKE2);
	// Add our test data
	result = blake2.AddData(&test);
	CHK(result == B_OK);

	result = blake2.Result(&hash);
	CHK(result == B_OK);

	CHK(hash.IsEmpty() != true);
	CHK(hash == knownHash);
}


CppUnit::Test*
CryptoHashTest::Suite()
{
	/* create our suite */
	CppUnit::TestSuite *suite = new CppUnit::TestSuite("CryptoHash");
	typedef CppUnit::TestCaller<CryptoHashTest> TC;

	/* add tests */
	suite->addTest(new TC("BCryptoHash::AddData(BString) Blake2 Test",
		&CryptoHashTest::Blake2HashTest));

	return suite;
}
