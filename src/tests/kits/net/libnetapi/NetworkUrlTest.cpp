/*
 * Copyright 2016-2018, Andrew Lindesay, apl@lindesay.co.nz.
 * Distributed under the terms of the MIT License.
 */


#include "NetworkUrlTest.h"

#include <Url.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


NetworkUrlTest::NetworkUrlTest()
{
}


NetworkUrlTest::~NetworkUrlTest()
{
}


void
NetworkUrlTest::setUp()
{
}


void
NetworkUrlTest::tearDown()
{
}


// #pragma mark - General Tests

/*
This is the "happy days" tests that checks that a URL featuring all of the
parsed elements successfully processes and the elements are present.
*/

void NetworkUrlTest::TestValidFullUrl()
{
	BURL url("http://ewe:pea@www.something.co.nz:8888/some/path?key1=value1#fragment");
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "http");
	CPPUNIT_ASSERT(url.HasProtocol());
	CPPUNIT_ASSERT(url.UserName() == "ewe");
	CPPUNIT_ASSERT(url.HasUserName());
	CPPUNIT_ASSERT(url.Password() == "pea");
	CPPUNIT_ASSERT(url.HasPassword());
	CPPUNIT_ASSERT(url.Host() == "www.something.co.nz");
	CPPUNIT_ASSERT(url.HasHost());
	CPPUNIT_ASSERT(url.Port() == 8888);
	CPPUNIT_ASSERT(url.HasPort());
	CPPUNIT_ASSERT(url.Path() == "/some/path");
	CPPUNIT_ASSERT(url.HasPath());
	CPPUNIT_ASSERT(url.Request() == "key1=value1");
	CPPUNIT_ASSERT(url.HasRequest());
	CPPUNIT_ASSERT(url.Fragment() == "fragment");
	CPPUNIT_ASSERT(url.HasFragment());
}


void NetworkUrlTest::TestHostWithPathAndFragment()
{
	BURL url("http://1.2.3.4/some/path#frag/ment");
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "http");
	CPPUNIT_ASSERT(url.HasProtocol());
	CPPUNIT_ASSERT(!url.HasUserName());
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(url.Host() == "1.2.3.4");
	CPPUNIT_ASSERT(url.HasHost());
	CPPUNIT_ASSERT(!url.HasPort());
	CPPUNIT_ASSERT(url.Path() == "/some/path");
	CPPUNIT_ASSERT(url.HasPath());
	CPPUNIT_ASSERT(!url.HasRequest());
	CPPUNIT_ASSERT(url.Fragment() == "frag/ment");
	CPPUNIT_ASSERT(url.HasFragment());
}


void NetworkUrlTest::TestHostWithFragment()
{
	BURL url("http://1.2.3.4#frag/ment");
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "http");
	CPPUNIT_ASSERT(url.HasProtocol());
	CPPUNIT_ASSERT(!url.HasUserName());
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(url.Host() == "1.2.3.4");
	CPPUNIT_ASSERT(url.HasHost());
	CPPUNIT_ASSERT(!url.HasPort());
	CPPUNIT_ASSERT(url.HasPath()); // see Url.cpp - evidently an empty path is still a path?
	CPPUNIT_ASSERT(!url.HasRequest());
	CPPUNIT_ASSERT(url.Fragment() == "frag/ment");
	CPPUNIT_ASSERT(url.HasFragment());
}


void NetworkUrlTest::TestIpv6HostPortPathAndRequest()
{
	BURL url("http://[123:a3:0:E3::123]:8080/some/path?key1=value1");
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "http");
	CPPUNIT_ASSERT(url.HasProtocol());
	CPPUNIT_ASSERT(!url.HasUserName());
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(url.Host() == "[123:a3:0:E3::123]");
	CPPUNIT_ASSERT(url.HasHost());
	CPPUNIT_ASSERT(url.Port() == 8080);
	CPPUNIT_ASSERT(url.HasPort());
	CPPUNIT_ASSERT(url.Path() == "/some/path");
	CPPUNIT_ASSERT(url.HasPath());
	CPPUNIT_ASSERT(url.Request() == "key1=value1");
	CPPUNIT_ASSERT(url.HasRequest());
	CPPUNIT_ASSERT(!url.HasFragment());
}


void NetworkUrlTest::TestFileUrl()
{
	BURL url("file:///northisland/wellington/brooklyn/windturbine");
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "file");
	CPPUNIT_ASSERT(url.HasProtocol());
	CPPUNIT_ASSERT(!url.HasUserName());
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(url.Host() == "");
	CPPUNIT_ASSERT(url.HasHost());
	CPPUNIT_ASSERT(!url.HasPort());
	CPPUNIT_ASSERT(url.Path() == "/northisland/wellington/brooklyn/windturbine");
	CPPUNIT_ASSERT(url.HasPath());
	CPPUNIT_ASSERT(!url.HasRequest());
	CPPUNIT_ASSERT(!url.HasFragment());
}


void NetworkUrlTest::TestDataUrl()
{
	BURL url("data:image/png;base64,iVBORw0KGI12P4//8/w38GIErkJggg==");
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "data");
	CPPUNIT_ASSERT(!url.HasUserName());
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(!url.HasHost());
	CPPUNIT_ASSERT(url.HasPath());
	CPPUNIT_ASSERT(url.Path() == "image/png;base64,iVBORw0KGI12P4//8/w38GIErkJggg==");
	CPPUNIT_ASSERT(!url.HasRequest());
	CPPUNIT_ASSERT(!url.HasFragment());
}


// #pragma mark - Authority Tests (UserName, Password, Host, Port)


void NetworkUrlTest::TestWithUserNameAndPasswordNoHostAndPort()
{
	BURL url("wierd://tea:tree@/x");
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "wierd");
	CPPUNIT_ASSERT(url.HasProtocol());
	CPPUNIT_ASSERT(url.UserName() == "tea");
	CPPUNIT_ASSERT(url.HasUserName());
	CPPUNIT_ASSERT(url.Password() == "tree");
	CPPUNIT_ASSERT(url.HasPassword());
	CPPUNIT_ASSERT(url.Host() == "");
	CPPUNIT_ASSERT(url.HasHost()); // any authority means there "is a host" - see SetAuthority comment.
	CPPUNIT_ASSERT(!url.HasPort());
	CPPUNIT_ASSERT(url.Path() == "/x");
	CPPUNIT_ASSERT(url.HasPath());
	CPPUNIT_ASSERT(!url.HasRequest());
	CPPUNIT_ASSERT(!url.HasFragment());
}


void NetworkUrlTest::TestHostAndPortWithNoUserNameAndPassword()
{
	BURL url("https://www.something.co.nz:443/z");
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "https");
	CPPUNIT_ASSERT(url.HasProtocol());
	CPPUNIT_ASSERT(!url.HasUserName());
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(url.Port() == 443);
	CPPUNIT_ASSERT(url.HasPort());
	CPPUNIT_ASSERT(url.Host() == "www.something.co.nz");
	CPPUNIT_ASSERT(url.HasHost());
	CPPUNIT_ASSERT(url.Path() == "/z");
	CPPUNIT_ASSERT(url.HasPath());
	CPPUNIT_ASSERT(!url.HasRequest());
	CPPUNIT_ASSERT(!url.HasFragment());
}


void NetworkUrlTest::TestHostWithNoPortOrUserNameAndPassword()
{
	BURL url("https://www.something.co.nz/z");
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "https");
	CPPUNIT_ASSERT(url.HasProtocol());
	CPPUNIT_ASSERT(!url.HasUserName());
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(url.Host() == "www.something.co.nz");
	CPPUNIT_ASSERT(url.HasHost());
	CPPUNIT_ASSERT(!url.HasPort());
	CPPUNIT_ASSERT(url.Path() == "/z");
	CPPUNIT_ASSERT(url.HasPath());
	CPPUNIT_ASSERT(!url.HasRequest());
	CPPUNIT_ASSERT(!url.HasFragment());
}


void NetworkUrlTest::TestHostWithNoPortNoPath()
{
	BURL url("https://www.something.co.nz");
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "https");
	CPPUNIT_ASSERT(url.HasProtocol());
	CPPUNIT_ASSERT(!url.HasUserName());
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(url.Host() == "www.something.co.nz");
	CPPUNIT_ASSERT(url.HasHost());
	CPPUNIT_ASSERT(!url.HasPort());
	CPPUNIT_ASSERT(!url.HasPath());
	CPPUNIT_ASSERT(!url.HasRequest());
	CPPUNIT_ASSERT(!url.HasFragment());
}


void NetworkUrlTest::TestHostWithPortNoPath()
{
	BURL url("https://www.something.co.nz:1234");
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "https");
	CPPUNIT_ASSERT(url.HasProtocol());
	CPPUNIT_ASSERT(!url.HasUserName());
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(url.Host() == "www.something.co.nz");
	CPPUNIT_ASSERT(url.HasHost());
	CPPUNIT_ASSERT(url.Port() == 1234);
	CPPUNIT_ASSERT(url.HasPort());
	CPPUNIT_ASSERT(!url.HasPath());
	CPPUNIT_ASSERT(!url.HasRequest());
	CPPUNIT_ASSERT(!url.HasFragment());
}


void NetworkUrlTest::TestHostWithEmptyPort()
{
	BURL url("https://www.something.co.nz:");
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "https");
	CPPUNIT_ASSERT(url.HasProtocol());
	CPPUNIT_ASSERT(!url.HasUserName());
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(url.Host() == "www.something.co.nz");
	CPPUNIT_ASSERT(url.HasHost());
	CPPUNIT_ASSERT(!url.HasPort());
	CPPUNIT_ASSERT(!url.HasPath());
	CPPUNIT_ASSERT(!url.HasRequest());
	CPPUNIT_ASSERT(!url.HasFragment());
}


void NetworkUrlTest::TestProtocol()
{
	BURL url("olala:");
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "olala");
	CPPUNIT_ASSERT(url.HasProtocol());
	CPPUNIT_ASSERT(!url.HasUserName());
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(!url.HasHost());
	CPPUNIT_ASSERT(!url.HasPort());
	CPPUNIT_ASSERT(!url.HasPath());
	CPPUNIT_ASSERT(!url.HasRequest());
	CPPUNIT_ASSERT(!url.HasFragment());
}


void NetworkUrlTest::TestMailTo()
{
	BURL url("mailto:eric@example.com");
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "mailto");
	CPPUNIT_ASSERT(url.HasProtocol());
	CPPUNIT_ASSERT(!url.HasUserName());
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(!url.HasHost());
	CPPUNIT_ASSERT(!url.HasPort());
	CPPUNIT_ASSERT(url.Path() == "eric@example.com");
	CPPUNIT_ASSERT(url.HasPath());
	CPPUNIT_ASSERT(!url.HasRequest());
	CPPUNIT_ASSERT(!url.HasFragment());
}


// #pragma mark - Various Authority Checks


void NetworkUrlTest::TestAuthorityNoUserName()
{
	BURL url("anything://:pwd@host");
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(!url.HasUserName());
	CPPUNIT_ASSERT(url.HasPassword());
	CPPUNIT_ASSERT(url.Password() == "pwd");
	CPPUNIT_ASSERT(url.HasHost());
	CPPUNIT_ASSERT(url.Host() == "host");
	CPPUNIT_ASSERT(!url.HasPort());
}


void NetworkUrlTest::TestAuthorityWithCredentialsSeparatorNoPassword()
{
	BURL url("anything://unam:@host");
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.HasUserName());
	CPPUNIT_ASSERT(url.UserName() == "unam");
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(url.HasHost());
	CPPUNIT_ASSERT(url.Host() == "host");
	CPPUNIT_ASSERT(!url.HasPort());
}


void NetworkUrlTest::TestAuthorityWithoutCredentialsSeparatorNoPassword()
{
	BURL url("anything://unam@host");
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.HasUserName());
	CPPUNIT_ASSERT(url.UserName() == "unam");
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(url.HasHost());
	CPPUNIT_ASSERT(url.Host() == "host");
	CPPUNIT_ASSERT(!url.HasPort());
}


void NetworkUrlTest::TestAuthorityBadPort()
{
	BURL url("anything://host:aaa");
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(!url.HasUserName());
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(url.HasHost());
	CPPUNIT_ASSERT(url.Host() == "host");
	CPPUNIT_ASSERT(!url.HasPort());
}


// #pragma mark - Invalid Forms


void NetworkUrlTest::TestWhitespaceBefore()
{
	BURL url("   https://www.something.co.nz/z");
	CPPUNIT_ASSERT(!url.IsValid());
}


void NetworkUrlTest::TestWhitespaceAfter()
{
	BURL url("https://www.something.co.nz/z\t\t ");
	CPPUNIT_ASSERT(!url.IsValid());
}


void NetworkUrlTest::TestWhitespaceMiddle()
{
	BURL url("https://www.  something.co.nz/z");
	CPPUNIT_ASSERT(!url.IsValid());
}


void NetworkUrlTest::TestHttpNoHost()
{
	BURL url("https:///z");
	CPPUNIT_ASSERT(!url.IsValid());
}


void NetworkUrlTest::TestEmpty()
{
	BURL url("");
	CPPUNIT_ASSERT(!url.IsValid());
}


// #pragma mark - Host validation


void NetworkUrlTest::TestBadHosts()
{
	CPPUNIT_ASSERT_MESSAGE("control check",
		BURL("http://host.example.com").IsValid());

	CPPUNIT_ASSERT_MESSAGE("hyphen in middle",
		(BURL("http://host.peppermint_tea.com").IsValid()));
	CPPUNIT_ASSERT_MESSAGE("dot at end",
		(BURL("http://host.camomile.co.nz.").IsValid()));
	CPPUNIT_ASSERT_MESSAGE("simple host",
		(BURL("http://tumeric").IsValid()));

	CPPUNIT_ASSERT_MESSAGE("idn domain encoded",
		(BURL("http://xn--bcher-kva.tld").IsValid()));
	CPPUNIT_ASSERT_MESSAGE("idn domain unencoded",
		(BURL("http://www.b\xc3\xbcch.at").IsValid()));

	CPPUNIT_ASSERT_MESSAGE("dot at start",
		!(BURL("http://.host.example.com").IsValid()));
	CPPUNIT_ASSERT_MESSAGE("double dot in domain",
		!(BURL("http://host.example..com").IsValid()));
	CPPUNIT_ASSERT_MESSAGE("double dot",
		!(BURL("http://host.example..com").IsValid()));
	CPPUNIT_ASSERT_MESSAGE("unexpected characters",
		!(BURL("http://<unexpected.characters>").IsValid()));
	CPPUNIT_ASSERT_MESSAGE("whitespace",
		!(BURL("http://host.exa ple.com").IsValid()));
}


// #pragma mark - Control


/*static*/ void
NetworkUrlTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("NetworkUrlTest");

	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestHostAndPortWithNoUserNameAndPassword",
		&NetworkUrlTest::TestHostAndPortWithNoUserNameAndPassword));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestWithUserNameAndPasswordNoHostAndPort",
		&NetworkUrlTest::TestWithUserNameAndPasswordNoHostAndPort));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestHostWithNoPortOrUserNameAndPassword",
		&NetworkUrlTest::TestHostWithNoPortOrUserNameAndPassword));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestHostWithNoPortNoPath",
		&NetworkUrlTest::TestHostWithNoPortNoPath));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestHostWithPortNoPath",
		&NetworkUrlTest::TestHostWithPortNoPath));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestHostWithEmptyPort",
		&NetworkUrlTest::TestHostWithEmptyPort));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestHostWithPathAndFragment",
		&NetworkUrlTest::TestHostWithPathAndFragment));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestHostWithFragment",
		&NetworkUrlTest::TestHostWithFragment));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestIpv6HostPortPathAndRequest",
		&NetworkUrlTest::TestIpv6HostPortPathAndRequest));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestProtocol",
		&NetworkUrlTest::TestProtocol));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestMailTo",
		&NetworkUrlTest::TestMailTo));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestDataUrl",
		&NetworkUrlTest::TestDataUrl));

	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestAuthorityNoUserName",
		&NetworkUrlTest::TestAuthorityNoUserName));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
        "NetworkUrlTest::TestAuthorityWithCredentialsSeparatorNoPassword",
        &NetworkUrlTest::TestAuthorityWithCredentialsSeparatorNoPassword));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
        "NetworkUrlTest::TestAuthorityWithoutCredentialsSeparatorNoPassword",
        &NetworkUrlTest::TestAuthorityWithoutCredentialsSeparatorNoPassword));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
        "NetworkUrlTest::TestAuthorityBadPort",
        &NetworkUrlTest::TestAuthorityBadPort));

	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestWhitespaceBefore",
		&NetworkUrlTest::TestWhitespaceBefore));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestWhitespaceAfter",
		&NetworkUrlTest::TestWhitespaceAfter));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestWhitespaceMiddle",
		&NetworkUrlTest::TestWhitespaceMiddle));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestEmpty",
		&NetworkUrlTest::TestEmpty));

	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestFileUrl",
		&NetworkUrlTest::TestFileUrl));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestValidFullUrl", &NetworkUrlTest::TestValidFullUrl));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestBadHosts", &NetworkUrlTest::TestBadHosts));

	parent.addTest("NetworkUrlTest", &suite);
}
