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
