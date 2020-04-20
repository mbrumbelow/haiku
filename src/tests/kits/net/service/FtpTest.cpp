/*
 * Copyright 2020, Haiku Inc.
 * Distributed under the terms of the MIT licence
 */


#include "FtpTest.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <posix/libgen.h>
#include <string>

#include <AutoDeleter.h>
#include <FtpConnection.h>
#include <NetworkKit.h>
#include <UrlProtocolListener.h>

#include <tools/cppunit/ThreadedTestCaller.h>

#include "TestServer.h"


namespace {

class TestListener : public BUrlProtocolListener {
public:
	TestListener(const std::string& expectedResponseBody)
		:
		fExpectedResponseBody(expectedResponseBody)
	{
	}

	virtual void DataReceived(
		BUrlRequest *caller,
		const char *data,
		off_t position,
		ssize_t size)
	{
		std::copy_n(
			data + position,
			size,
			std::back_inserter(fActualResponseBody));
	}


	void Verify()
	{
		CPPUNIT_ASSERT_EQUAL(fExpectedResponseBody, fActualResponseBody);
	}

private:
	std::string fExpectedResponseBody;
	std::string fActualResponseBody;
};


template <typename T>
void AddCommonTests(BThreadedTestCaller<T>& testCaller)
{
	testCaller.addThread("GetWorkingDirectoryTest",
						 &T::GetWorkingDirectoryTest);
}

}


FtpTest::FtpTest(TestServerMode mode)
	:
	fTestServer(mode)
{
}


FtpTest::~FtpTest()
{
}


void
FtpTest::setUp()
{
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"Starting up test server",
		B_OK,
		fTestServer.Start());
}


void
FtpTest::GetWorkingDirectoryTest()
{
	std::string expectedResponseBody("257 \"/\" is the current "
		"working directory.\r\n");
	TestListener listener(expectedResponseBody);

	BUrl testUrl(fTestServer.BaseUrl());

	BFtpConnection request(testUrl);
	request.SetListener(&listener);
	request.GetWorkingDirectory();

	CPPUNIT_ASSERT(request.Run());

	while (request.IsRunning())
		snooze(1000);

	CPPUNIT_ASSERT_EQUAL(B_OK, request.Status());

	const BFtpDirectoryResponse &result =
		dynamic_cast<const BFtpDirectoryResponse &>
			(request.Result());
	CPPUNIT_ASSERT_EQUAL(257, result.GetStatus());
	CPPUNIT_ASSERT_EQUAL(expectedResponseBody, result.GetMessage().String());
	CPPUNIT_ASSERT_EQUAL(BString("/"), result.GetDirectory());

	listener.Verify();
}


/* static */ void
FtpTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite *suite = new CppUnit::TestSuite("FtpTest");

	FtpTest* ftpTest = new FtpTest();
	BThreadedTestCaller<FtpTest> *ftpTestCaller
		= new BThreadedTestCaller<FtpTest>("FtpTest::", ftpTest);

	AddCommonTests<FtpTest>(*ftpTestCaller);

	suite->addTest(ftpTestCaller);
	parent.addTest("FtpTest", suite);
}
