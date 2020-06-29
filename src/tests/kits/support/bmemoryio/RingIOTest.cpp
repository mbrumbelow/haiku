#include "RingIOTest.h"

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
RingIOTest::Suite() {
	CppUnit::TestSuite *suite = new CppUnit::TestSuite("RingIOTest");
	BThreadedTestCaller<RingIOTest> *caller;

	RingIOTest *big = new RingIOTest(sizeof(BIG_PAYLOAD));
	caller = new BThreadedTestCaller<RingIOTest>("RingIOTest: RW threaded, big buffer", big);
	caller->addThread("WR", &RingIOTest::WriteTest);
	caller->addThread("RD", &RingIOTest::ReadTest);
	suite->addTest(caller);

	RingIOTest *full = new RingIOTest(sizeof(FULL_PAYLOAD));
	caller = new BThreadedTestCaller<RingIOTest>("RingIOTest: RW threaded, medium buffer", full);
	caller->addThread("WR", &RingIOTest::WriteTest);
	caller->addThread("RD", &RingIOTest::ReadTest);
	suite->addTest(caller);

	RingIOTest *small = new RingIOTest(sizeof(SMALL_PAYLOAD));
	caller = new BThreadedTestCaller<RingIOTest>("RingIOTest: RW threaded, small buffer", small);
	caller->addThread("WR", &RingIOTest::WriteTest);
	caller->addThread("RD", &RingIOTest::ReadTest);
	suite->addTest(caller);

	RingIOTest *single = new RingIOTest(0);
	suite->addTest(new CppUnit::TestCaller<RingIOTest>("RingIOTest: RW single threaded with resizing", &RingIOTest::ReadWriteSingleTest, single));
	suite->addTest(new CppUnit::TestCaller<RingIOTest>("RingIOTest: Attempt to truncate buffer", &RingIOTest::InvalidResizeTest, single));

	return suite;
}


static status_t
WriteExactly(BRingIO& ring, const void* _buffer, size_t size)
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
ReadExactly(BRingIO& ring, void* _buffer, size_t size)
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
ReadCheck(BRingIO& ring, const void* cmp, size_t size)
{
	char *buffer = new char[size];
	memset(buffer, 0, size);
	CHK(ReadExactly(ring, buffer, size) == B_OK);
	CHK(memcmp(buffer, cmp, size) == 0);
}


void
RingIOTest::WriteTest()
{
	CHK(fRing.InitCheck() == B_OK);

	CHK(WriteExactly(fRing, SMALL_PAYLOAD, sizeof(SMALL_PAYLOAD)) == B_OK);
	CHK(WriteExactly(fRing, FULL_PAYLOAD, sizeof(FULL_PAYLOAD)) == B_OK);
	CHK(WriteExactly(fRing, BIG_PAYLOAD, sizeof(BIG_PAYLOAD)) == B_OK);
}


void
RingIOTest::ReadTest()
{
	CHK(fRing.InitCheck() == B_OK);

	ReadCheck(fRing, SMALL_PAYLOAD, sizeof(SMALL_PAYLOAD));
	ReadCheck(fRing, FULL_PAYLOAD, sizeof(FULL_PAYLOAD));
	ReadCheck(fRing, BIG_PAYLOAD, sizeof(BIG_PAYLOAD));
}


void
RingIOTest::ReadWriteSingleTest()
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
RingIOTest::InvalidResizeTest()
{
	CHK(fRing.SetSize(sizeof(FULL_PAYLOAD)) == B_OK);
	CHK(WriteExactly(fRing, FULL_PAYLOAD, sizeof(FULL_PAYLOAD)) == B_OK);
	CHK(fRing.SetSize(0) == B_NO_MEMORY);
}
