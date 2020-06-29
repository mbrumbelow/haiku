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
	void BusyWriterTest();
	void BusyReaderTest();
	void ReadWriteSingleTest();
	void InvalidResizeTest();
	void TimeoutTest();

protected:
	void _EndOnFullBuffer();
	void _EndOnEmptyBuffer();
	BMemoryRingIO fRing;
};

#endif // _MEMORY_RING_IO_TEST_H_
