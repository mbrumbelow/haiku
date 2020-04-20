/*
 * Copyright 2020 Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef FTP_TEST_H
#define FTP_TEST_H


#include <Url.h>

#include <TestCase.h>
#include <TestSuite.h>

#include <cppunit/TestSuite.h>
#include <tools/cppunit/ThreadedTestCase.h>

#include "TestServer.h"


class FtpTest: public BThreadedTestCase {
public:
						FtpTest(TestServerMode mode = TEST_SERVER_MODE_FTP);
	virtual				~FtpTest();

	virtual	void		setUp();

			void		GetWorkingDirectoryTest();

	static	void		AddTests(BTestSuite& suite);

private:
			TestServer	fTestServer;
};


#endif
