/*
 * Copyright 2021-2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "CryptoHashTest.h"

#include <crypto/CryptoHash.h>


BTestSuite* getTestSuite() {
	BTestSuite *suite = new BTestSuite("Crypto");

	// ##### Add test suites here #####
	suite->addTest("BCryptoHash", CryptoHashTest::Suite());

	return suite;
}
