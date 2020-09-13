#include <TestSuite.h>
#include <TestSuiteAddon.h>

// ##### Include headers for your tests here #####
#include "cryptohash/CryptoHash.h"

BTestSuite* getTestSuite2() {
        BTestSuite *suite = new BTestSuite("Crypto");

        // ##### Add test suites here #####
        suite->addTest("BCryptoHash", CryptoHashTestSuite());
}
