#include "CryptoHashTest.h"
#include "CryptoHashBlake2Tester.h"


CppUnit::Test* CryptoHashTestSuite()
{
        CppUnit::TestSuite *testSuite = new CppUnit::TestSuite();

	testSuite->addTest(CryptoHashBlake2Tester::Suite());

        return testSuite;
}
