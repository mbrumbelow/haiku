#ifndef __CRYPTOHASH_TEST
#define __CRYPTOHASH_TEST

#include <crypto/CryptoHash.h>

/** CppUnit support */
#include <TestCase.h>

class CryptoHashTest : public BTestCase {
public:
    CryptoHashTest(std::string name = "");
    ~CryptoHashTest();
    
	/* cppunit suite function prototype */    
    static CppUnit::Test *Suite();    
    
    //actual tests
	void Blake2HashTest();
private:
};

#endif
