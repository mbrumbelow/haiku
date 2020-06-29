#include "MemoryRingIOTest.h"

#include <ThreadedTestCaller.h>
#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <stdio.h>
#include <string.h>
#include <kernel/OS.h>
#include <TestUtils.h>

#define BIG_PAYLOAD \
	"a really long string that can fill the buffer multiple times"
#define FULL_PAYLOAD "16 characters xx"
#define SMALL_PAYLOAD "shorter"

CppUnit::Test*
MemoryRingIOTest::Suite() {
	CppUnit::TestSuite *suite = new CppUnit::TestSuite("MemoryRingIOTest");
	BThreadedTestCaller<MemoryRingIOTest> *caller;

	MemoryRingIOTest *big = new MemoryRingIOTest(sizeof(BIG_PAYLOAD));
	caller = new BThreadedTestCaller<MemoryRingIOTest>(
		"MemoryRingIOTest: RW threaded, big buffer", big);
	caller->addThread("WR", &MemoryRingIOTest::WriteTest);
	caller->addThread("RD", &MemoryRingIOTest::ReadTest);
	suite->addTest(caller);

	MemoryRingIOTest *full = new MemoryRingIOTest(sizeof(FULL_PAYLOAD));
	caller = new BThreadedTestCaller<MemoryRingIOTest>(
		"MemoryRingIOTest: RW threaded, medium buffer", full);
	caller->addThread("WR", &MemoryRingIOTest::WriteTest);
	caller->addThread("RD", &MemoryRingIOTest::ReadTest);
	suite->addTest(caller);

	MemoryRingIOTest *small = new MemoryRingIOTest(sizeof(SMALL_PAYLOAD));
	caller = new BThreadedTestCaller<MemoryRingIOTest>(
		"MemoryRingIOTest: RW threaded, small buffer", small);
	caller->addThread("WR", &MemoryRingIOTest::WriteTest);
	caller->addThread("RD", &MemoryRingIOTest::ReadTest);
	suite->addTest(caller);

	MemoryRingIOTest *endWrite = new MemoryRingIOTest(sizeof(FULL_PAYLOAD));
	caller = new BThreadedTestCaller<MemoryRingIOTest>(
		"MemoryRingIOTest: RW threaded, reader set end reached on writer wait",
		endWrite);
	caller->addThread("WR", &MemoryRingIOTest::BusyWriterTest);
	caller->addThread("RD", &MemoryRingIOTest::_EndOnFullBuffer);
	suite->addTest(caller);

	MemoryRingIOTest *endRead = new MemoryRingIOTest(sizeof(FULL_PAYLOAD));
	caller = new BThreadedTestCaller<MemoryRingIOTest>(
		"MemoryRingIOTest: RW threaded, writer set end reached on reader wait",
		endRead);
	caller->addThread("RD", &MemoryRingIOTest::BusyReaderTest);
	caller->addThread("WR", &MemoryRingIOTest::_EndOnEmptyBuffer);
	suite->addTest(caller);

	MemoryRingIOTest *single = new MemoryRingIOTest(0);
	suite->addTest(new CppUnit::TestCaller<MemoryRingIOTest>(
		"MemoryRingIOTest: RW single threaded with resizing",
		&MemoryRingIOTest::ReadWriteSingleTest, single));
	suite->addTest(new CppUnit::TestCaller<MemoryRingIOTest>(
		"MemoryRingIOTest: Attempt to truncate buffer",
		&MemoryRingIOTest::InvalidResizeTest, single));

	return suite;
}


static void
ReadCheck(BMemoryRingIO& ring, const void* cmp, size_t size)
{
	char *buffer = new char[size];
	memset(buffer, 0, size);
	size_t read;
	CHK(ring.ReadExactly(buffer, size, &read) == B_OK);
	CHK(read == size);
	CHK(memcmp(buffer, cmp, size) == 0);
}


void
MemoryRingIOTest::WriteTest()
{
	CHK(fRing.InitCheck() == B_OK);

	CHK(fRing.WriteExactly(SMALL_PAYLOAD, sizeof(SMALL_PAYLOAD), NULL) == B_OK);
	CHK(fRing.WriteExactly(FULL_PAYLOAD, sizeof(FULL_PAYLOAD), NULL) == B_OK);
	CHK(fRing.WriteExactly(BIG_PAYLOAD, sizeof(BIG_PAYLOAD), NULL) == B_OK);
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
MemoryRingIOTest::BusyWriterTest()
{
	CHK(fRing.InitCheck() == B_OK);
	CHK(fRing.GetSize() < sizeof(BIG_PAYLOAD));

	CHK(fRing.WriteExactly(BIG_PAYLOAD, sizeof(BIG_PAYLOAD), NULL)
		== B_DEVICE_FULL);
}


void
MemoryRingIOTest::BusyReaderTest()
{
	CHK(fRing.InitCheck() == B_OK);

	char buffer[100];
	CHK(fRing.Read(buffer, sizeof(buffer)) == 0);
}


void
MemoryRingIOTest::ReadWriteSingleTest()
{
	CHK(fRing.SetSize(sizeof(BIG_PAYLOAD)) == B_OK);
	CHK(fRing.WriteExactly(BIG_PAYLOAD, sizeof(BIG_PAYLOAD)) == B_OK);
	ReadCheck(fRing, BIG_PAYLOAD, sizeof(BIG_PAYLOAD));

	CHK(fRing.SetSize(sizeof(FULL_PAYLOAD)) == B_OK);
	CHK(fRing.WriteExactly(FULL_PAYLOAD, sizeof(FULL_PAYLOAD)) == B_OK);
	ReadCheck(fRing, FULL_PAYLOAD, sizeof(FULL_PAYLOAD));

	CHK(fRing.SetSize(sizeof(SMALL_PAYLOAD)) == B_OK);
	CHK(fRing.WriteExactly(SMALL_PAYLOAD, sizeof(SMALL_PAYLOAD)) == B_OK);
	ReadCheck(fRing, SMALL_PAYLOAD, sizeof(SMALL_PAYLOAD));
}


void
MemoryRingIOTest::InvalidResizeTest()
{
	CHK(fRing.SetSize(sizeof(FULL_PAYLOAD)) == B_OK);
	CHK(fRing.WriteExactly(FULL_PAYLOAD, sizeof(FULL_PAYLOAD)) == B_OK);
	CHK(fRing.SetSize(0) == B_NO_MEMORY);
}


void
MemoryRingIOTest::_EndOnFullBuffer()
{
	CHK(fRing.InitCheck() == B_OK);

	while (fRing.SpaceAvailable() > 0)
		fRing.WaitForRead();

	/* snooze for sometime to ensure that the other thread entered
	 * WaitForWrite().
	 */
	snooze(1000);
	/* this should unblock the other thread */
	fRing.SetEndReached(true);
}


void
MemoryRingIOTest::_EndOnEmptyBuffer()
{
	CHK(fRing.InitCheck() == B_OK);

	while (fRing.BytesAvailable() > 0)
		fRing.WaitForWrite();

	/* snooze for sometime to ensure that the other thread entered
	 * WaitForRead().
	 */
	snooze(1000);
	/* this should unblock the other thread */
	fRing.SetEndReached(true);
}
