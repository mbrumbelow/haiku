#include "MemoryRingIOTest.h"

#include <ThreadedTestCaller.h>
#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <stdio.h>
#include <string.h>
#include <kernel/OS.h>
#include <TestUtils.h>

#define BIG_PAYLOAD "a really long string that can fill the buffer multiple times"
#define FULL_PAYLOAD "16 characters xx"
#define SMALL_PAYLOAD "shorter"

CppUnit::Test*
MemoryRingIOTest::Suite() {
	CppUnit::TestSuite *suite = new CppUnit::TestSuite("MemoryRingIOTest");
	BThreadedTestCaller<MemoryRingIOTest> *caller;

	MemoryRingIOTest *big = new MemoryRingIOTest(sizeof(BIG_PAYLOAD));
	caller = new BThreadedTestCaller<MemoryRingIOTest>("MemoryRingIOTest: RW threaded, big buffer", big);
	caller->addThread("WR", &MemoryRingIOTest::WriteTest);
	caller->addThread("RD", &MemoryRingIOTest::ReadTest);
	suite->addTest(caller);

	MemoryRingIOTest *full = new MemoryRingIOTest(sizeof(FULL_PAYLOAD));
	caller = new BThreadedTestCaller<MemoryRingIOTest>("MemoryRingIOTest: RW threaded, medium buffer", full);
	caller->addThread("WR", &MemoryRingIOTest::WriteTest);
	caller->addThread("RD", &MemoryRingIOTest::ReadTest);
	suite->addTest(caller);

	MemoryRingIOTest *small = new MemoryRingIOTest(sizeof(SMALL_PAYLOAD));
	caller = new BThreadedTestCaller<MemoryRingIOTest>("MemoryRingIOTest: RW threaded, small buffer", small);
	caller->addThread("WR", &MemoryRingIOTest::WriteTest);
	caller->addThread("RD", &MemoryRingIOTest::ReadTest);
	suite->addTest(caller);

	MemoryRingIOTest *single = new MemoryRingIOTest(0);
	suite->addTest(new CppUnit::TestCaller<MemoryRingIOTest>("MemoryRingIOTest: RW single threaded with resizing", &MemoryRingIOTest::ReadWriteSingleTest, single));
	suite->addTest(new CppUnit::TestCaller<MemoryRingIOTest>("MemoryRingIOTest: Attempt to truncate buffer", &MemoryRingIOTest::InvalidResizeTest, single));

	return suite;
}


static status_t
WriteExactly(BMemoryRingIO& ring, const void* _buffer, size_t size)
{
	const char* buffer = reinterpret_cast<const char*>(_buffer);
	size_t written = 0;
	while (written < size) {
		ring.WaitForWrite();
		ssize_t write = ring.Write(&buffer[written], size - written);
		if (write < 0)
			return write;
		written += write;
	}
	return B_OK;
}


static status_t
ReadExactly(BMemoryRingIO& ring, void* _buffer, size_t size)
{
	char* buffer = reinterpret_cast<char*>(_buffer);
	size_t read = 0;
	while (read < size) {
		ring.WaitForRead();
		ssize_t r = ring.Read(&buffer[read], size - read);
		if (r < 0)
			return r;
		read += r;
	}
	return B_OK;
}


static void
ReadCheck(BMemoryRingIO& ring, const void* cmp, size_t size)
{
	char *buffer = new char[size];
	memset(buffer, 0, size);
	CHK(ReadExactly(ring, buffer, size) == B_OK);
	CHK(memcmp(buffer, cmp, size) == 0);
}


void
MemoryRingIOTest::WriteTest()
{
	CHK(fRing.InitCheck() == B_OK);

	CHK(WriteExactly(fRing, SMALL_PAYLOAD, sizeof(SMALL_PAYLOAD)) == B_OK);
	CHK(WriteExactly(fRing, FULL_PAYLOAD, sizeof(FULL_PAYLOAD)) == B_OK);
	CHK(WriteExactly(fRing, BIG_PAYLOAD, sizeof(BIG_PAYLOAD)) == B_OK);
}


void
MemoryRingIOTest::ReadTest()
{
	CHK(fRing.InitCheck() == B_OK);

	ReadCheck(fRing, SMALL_PAYLOAD, sizeof(SMALL_PAYLOAD));
	ReadCheck(fRing, FULL_PAYLOAD, sizeof(FULL_PAYLOAD));
	ReadCheck(fRing, BIG_PAYLOAD, sizeof(BIG_PAYLOAD));
}


void
MemoryRingIOTest::ReadWriteSingleTest()
{
	CHK(fRing.SetSize(sizeof(BIG_PAYLOAD)) == B_OK);
	CHK(WriteExactly(fRing, BIG_PAYLOAD, sizeof(BIG_PAYLOAD)) == B_OK);
	ReadCheck(fRing, BIG_PAYLOAD, sizeof(BIG_PAYLOAD));

	CHK(fRing.SetSize(sizeof(FULL_PAYLOAD)) == B_OK);
	CHK(WriteExactly(fRing, FULL_PAYLOAD, sizeof(FULL_PAYLOAD)) == B_OK);
	ReadCheck(fRing, FULL_PAYLOAD, sizeof(FULL_PAYLOAD));

	CHK(fRing.SetSize(sizeof(SMALL_PAYLOAD)) == B_OK);
	CHK(WriteExactly(fRing, SMALL_PAYLOAD, sizeof(SMALL_PAYLOAD)) == B_OK);
	ReadCheck(fRing, SMALL_PAYLOAD, sizeof(SMALL_PAYLOAD));
}


void
MemoryRingIOTest::InvalidResizeTest()
{
	CHK(fRing.SetSize(sizeof(FULL_PAYLOAD)) == B_OK);
	CHK(WriteExactly(fRing, FULL_PAYLOAD, sizeof(FULL_PAYLOAD)) == B_OK);
	CHK(fRing.SetSize(0) == B_NO_MEMORY);
}
