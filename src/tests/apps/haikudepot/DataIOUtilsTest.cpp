/*
 * Copyright 2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "DataIOUtilsTest.h"

#include <String.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include <string.h>

#include "DataIOUtils.h"

// This is 24 x 10 bytes.
const char* kSampleData =
	"0123456789012345678901234567890123456789"
	"0123456789012345678901234567890123456789"
	"0123456789012345678901234567890123456789"
	"0123456789012345678901234567890123456789"
	"0123456789012345678901234567890123456789"
	"0123456789012345678901234567890123456789"
	"0123456789012345678901234567890123456789"
	"0123456789012345678901234567890123456789";


DataIOUtilsTest::DataIOUtilsTest()
{
}


DataIOUtilsTest::~DataIOUtilsTest()
{
}


void
DataIOUtilsTest::TestReadBase64JwtClaims_1()
{
	const char* jwtToken = "eyJpc3MiOiJkZXYuaGRzIiwic3ViIjoiZXJpazY0QGhkcyIs"
		"ImV4cCI6MTY5MzE5MTMzMiwiaWF0IjoxNjkzMTkxMDMyfQ";
	BMemoryIO memoryIo(jwtToken, strlen(jwtToken));
	Base64DecodingDataIO base64DecodingIo(&memoryIo, '-', '_');
	char actualOutputBuffer[71];
	size_t actualReadBytes;

	bzero(actualOutputBuffer, 71);

// ----------------------
	status_t result = base64DecodingIo.ReadExactly(actualOutputBuffer, 70, &actualReadBytes);
// ----------------------

	CPPUNIT_ASSERT_EQUAL(B_OK, result);
	CPPUNIT_ASSERT_EQUAL(70, actualReadBytes);
	actualOutputBuffer[actualReadBytes] = 0;

	CPPUNIT_ASSERT_EQUAL(0x7b, (uint8) actualOutputBuffer[0]);

	CPPUNIT_ASSERT_EQUAL(
		BString("{\"iss\":\"dev.hds\",\"sub\":\"erik64@hds\",\"exp\":1693191332,\"iat\""
			":1693191032}"),
		BString(actualOutputBuffer));
}


void
DataIOUtilsTest::TestReadBase64JwtClaims_2()
{
	const char* jwtToken = "eyJpc3MiOiJkZXYuaGRzIiwic3ViIjoidG93ZWxkb3dudGVhQ"
		"GhkcyIsImV4cCI6MTY5MzczODgyNiwiaWF0IjoxNjkzNzM4NTI2fQ";
	BMemoryIO memoryIo(jwtToken, strlen(jwtToken));
	Base64DecodingDataIO base64DecodingIo(&memoryIo, '-', '_');
	char actualOutputBuffer[77];
	size_t actualReadBytes;

	bzero(actualOutputBuffer, 77);

// ----------------------
	status_t result = base64DecodingIo.ReadExactly(actualOutputBuffer, 76, &actualReadBytes);
// ----------------------

	CPPUNIT_ASSERT_EQUAL(B_OK, result);
	CPPUNIT_ASSERT_EQUAL(76, actualReadBytes);
	actualOutputBuffer[actualReadBytes] = 0;

	CPPUNIT_ASSERT_EQUAL(0x7b, (uint8) actualOutputBuffer[0]);

	CPPUNIT_ASSERT_EQUAL(
		BString("{\"iss\":\"dev.hds\",\"sub\":\"toweldowntea@hds\",\"exp\":1693738826,\"iat\""
			":1693738526}"),
		BString(actualOutputBuffer));
}


void
DataIOUtilsTest::TestReadBase64Corrupt()
{
	const char* jwtToken = "QW5k$mV3";
		// note that '$' is not a valid base64 character
	BMemoryIO memoryIo(jwtToken, strlen(jwtToken));
	Base64DecodingDataIO base64DecodingIo(&memoryIo, '-', '_');
	char actualOutputBuffer[7];
	size_t actualReadBytes;

	bzero(actualOutputBuffer, 7);

// ----------------------
	status_t result = base64DecodingIo.ReadExactly(actualOutputBuffer, 6, &actualReadBytes);
// ----------------------

	CPPUNIT_ASSERT(B_OK != result);
}


void
DataIOUtilsTest::TestBufferedAcrossDelegateReads()
{
	BMemoryIO memoryIo(kSampleData, strlen(kSampleData));
	BufferedDataIO bufferedDataIo(&memoryIo, 10);
	char actualOutputBuffer[25];
	size_t actualReadBytes;

	bzero(actualOutputBuffer, 25);

	// ----------------------
    status_t result = bufferedDataIo.ReadExactly(actualOutputBuffer, 25, &actualReadBytes);
    // ----------------------

	CPPUNIT_ASSERT_EQUAL(B_OK, result);
	CPPUNIT_ASSERT_EQUAL(25, actualReadBytes);
	CPPUNIT_ASSERT_EQUAL(0, memcmp(actualOutputBuffer, kSampleData, 25));
}


void
DataIOUtilsTest::TestBufferedByteByByte()
{
	BMemoryIO memoryIo(kSampleData, strlen(kSampleData));
	BufferedDataIO bufferedDataIo(&memoryIo, 10);
	char actualOutputBuffer[25];

	bzero(actualOutputBuffer, 25);

	// ----------------------
	for (int i = 0; i < 25; i++) {
    	size_t result = bufferedDataIo.Read(&actualOutputBuffer[i], 1);
    	CPPUNIT_ASSERT_EQUAL(1, result);
    }
    // ----------------------

	CPPUNIT_ASSERT_EQUAL(0, memcmp(actualOutputBuffer, kSampleData, 25));
}


void
DataIOUtilsTest::TestBufferedZeroLengthRead()
{
	BMemoryIO memoryIo(kSampleData, strlen(kSampleData));
	BufferedDataIO bufferedDataIo(&memoryIo, 10);
	char actualOutputBuffer[25];

	bzero(actualOutputBuffer, 25);

	// ----------------------
    size_t result = bufferedDataIo.Read(actualOutputBuffer, 0);
    // ----------------------

	CPPUNIT_ASSERT_EQUAL(0, result);
}


void
DataIOUtilsTest::TestBufferedOverflowRead()
{
	BMemoryIO memoryIo(kSampleData, 20);
	BufferedDataIO bufferedDataIo(&memoryIo, 10);
	char actualOutputBuffer[25];
	size_t actualReadBytes;

	bzero(actualOutputBuffer, 25);

	// ----------------------
    status_t result = bufferedDataIo.ReadExactly(actualOutputBuffer, 25, &actualReadBytes);
    // ----------------------

	CPPUNIT_ASSERT_EQUAL(B_PARTIAL_READ, result);
	CPPUNIT_ASSERT_EQUAL(20, actualReadBytes);
	CPPUNIT_ASSERT_EQUAL(0, memcmp(actualOutputBuffer, kSampleData, 20));
}


/*static*/ void
DataIOUtilsTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("DataIOUtilsTest");

	suite.addTest(
		new CppUnit::TestCaller<DataIOUtilsTest>(
			"DataIOUtilsTest::TestReadBase64JwtClaims_1",
			&DataIOUtilsTest::TestReadBase64JwtClaims_1));

	suite.addTest(
		new CppUnit::TestCaller<DataIOUtilsTest>(
			"DataIOUtilsTest::TestReadBase64JwtClaims_2",
			&DataIOUtilsTest::TestReadBase64JwtClaims_2));

	suite.addTest(
		new CppUnit::TestCaller<DataIOUtilsTest>(
			"DataIOUtilsTest::TestReadBase64Corrupt",
			&DataIOUtilsTest::TestReadBase64Corrupt));

	suite.addTest(
		new CppUnit::TestCaller<DataIOUtilsTest>(
			"DataIOUtilsTest::TestBufferedAcrossDelegateReads",
			&DataIOUtilsTest::TestBufferedAcrossDelegateReads));

	suite.addTest(
		new CppUnit::TestCaller<DataIOUtilsTest>(
			"DataIOUtilsTest::TestBufferedByteByByte",
			&DataIOUtilsTest::TestBufferedByteByByte));

	suite.addTest(
		new CppUnit::TestCaller<DataIOUtilsTest>(
			"DataIOUtilsTest::TestBufferedZeroLengthRead",
			&DataIOUtilsTest::TestBufferedZeroLengthRead));

	suite.addTest(
		new CppUnit::TestCaller<DataIOUtilsTest>(
			"DataIOUtilsTest::TestBufferedOverflowRead",
			&DataIOUtilsTest::TestBufferedOverflowRead));

	parent.addTest("DataIOUtilsTest", &suite);
}