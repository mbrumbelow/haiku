/*
 * Copyright 2014-2020 Haiku, inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef HTTP_TEST_H
#define HTTP_TEST_H


#include <Url.h>

#include <TestCase.h>
#include <TestSuite.h>

#include <cppunit/TestSuite.h>

#include "TestServer.h"


class HttpTest: public BTestCase {
public:
											HttpTest(TestServerMode mode
												= TEST_SERVER_MODE_HTTP);
	virtual									~HttpTest();

	virtual						void		setUp();

								void		GetTest();
								void		UploadTest();
								void		AuthBasicTest();
								void		AuthDigestTest();
								void		ProxyTest();

	static						void		AddTests(BTestSuite& suite);

private:
	template<class T> static	void		_AddCommonTests(BString prefix,
												CppUnit::TestSuite& suite);

								TestServer	fTestServer;
};


class HttpsTest: public HttpTest {
public:
								HttpsTest();
};


#endif
