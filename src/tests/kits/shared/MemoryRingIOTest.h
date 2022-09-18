/*
 * Copyright 2022 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Leorize, leorize+oss@disroot.org
 */
#ifndef _MEMORY_RING_IO_TEST_H
#define _MEMORY_RING_IO_TEST_H


#include <ThreadedTestCase.h>
#include <MemoryRingIO.h>

class BTestSuite;


class MemoryRingIOTest : public BThreadedTestCase
{
public:
	MemoryRingIOTest(size_t bufferSize) : fRing(bufferSize) {};

	void WriteTest();
	void ReadTest();
	void BusyWriterTest();
	void BusyReaderTest();
	void ReadWriteSingleTest();
	void InvalidResizeTest();
	void TimeoutTest();

	static void AddTests(BTestSuite& parent);


protected:
	void _DisableWriteOnFullBuffer();
	void _DisableWriteOnEmptyBuffer();

	BMemoryRingIO fRing;
};

#endif // _MEMORY_RING_IO_TEST_H
