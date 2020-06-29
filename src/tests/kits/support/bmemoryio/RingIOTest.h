#ifndef _RING_IO_TEST_H_
#define _RING_IO_TEST_H_

#include <ThreadedTestCase.h>
#include <RingIO.h>

class RingIOTest : public BThreadedTestCase {
public:
	RingIOTest(size_t bufferSize) : fRing(bufferSize) {};

	static CppUnit::Test* Suite();

	void WriteTest();
	void ReadTest();
	void ReadWriteSingleTest();
	void InvalidResizeTest();

protected:
	BRingIO fRing;
};

#endif // _example_test_h_
