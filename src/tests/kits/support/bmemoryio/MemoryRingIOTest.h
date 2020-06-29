#ifndef _MEMORY_RING_IO_TEST_H_
#define _MEMORY_RING_IO_TEST_H_

#include <ThreadedTestCase.h>
#include <MemoryRingIO.h>

class MemoryRingIOTest : public BThreadedTestCase {
public:
	MemoryRingIOTest(size_t bufferSize) : fRing(bufferSize) {};

	static CppUnit::Test* Suite();

	void WriteTest();
	void ReadTest();
	void ReadWriteSingleTest();
	void InvalidResizeTest();

protected:
	BMemoryRingIO fRing;
};

#endif // _MEMORY_RING_IO_TEST_H_
