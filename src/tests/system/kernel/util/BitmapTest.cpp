#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <stdio.h>
#include <TestUtils.h>

#define ASSERT CPPUNIT_ASSERT

#include "BitmapTest.h"
#include "Bitmap.h"

BitmapTest::BitmapTest(std::string name)
	: BTestCase(name)
{
}

CppUnit::Test*
BitmapTest::Suite()
{
	CppUnit::TestSuite *suite = new CppUnit::TestSuite("Bitmap");

	suite->addTest(new CppUnit::TestCaller<BitmapTest>("Bitmap::Resize test",
		&BitmapTest::ResizeTest));
	suite->addTest(new CppUnit::TestCaller<BitmapTest>("Bitmap::Shift test",
		&BitmapTest::ShiftTest));

	return suite;
}

void
BitmapTest::ResizeTest()
{
	BKernel::Bitmap bitmap(10);
	bitmap.Set(6);

	CPPUNIT_ASSERT(bitmap.Get(6));
	CPPUNIT_ASSERT(!bitmap.Get(5));
	CPPUNIT_ASSERT(!bitmap.Get(7));

	bitmap.Resize(20);

	CPPUNIT_ASSERT(bitmap.Get(6));
	CPPUNIT_ASSERT(!bitmap.Get(7));
	CPPUNIT_ASSERT(!bitmap.Get(19));
}

void
BitmapTest::ShiftTest()
{
	BKernel::Bitmap bitmap(10);
	bitmap.Set(6);

	CPPUNIT_ASSERT(bitmap.Get(6));
	CPPUNIT_ASSERT(!bitmap.Get(5));
	CPPUNIT_ASSERT(!bitmap.Get(7));

	bitmap.Shift(10);

	CPPUNIT_ASSERT(bitmap.Get(16));
	CPPUNIT_ASSERT(!bitmap.Get(15));
	CPPUNIT_ASSERT(!bitmap.Get(17));
	CPPUNIT_ASSERT(!bitmap.Get(6));

	bitmap.Shift(-9);

	CPPUNIT_ASSERT(bitmap.Get(7));
	CPPUNIT_ASSERT(!bitmap.Get(6));
	CPPUNIT_ASSERT(!bitmap.Get(8));
	CPPUNIT_ASSERT(!bitmap.Get(6));
	CPPUNIT_ASSERT(!bitmap.Get(16));
}
