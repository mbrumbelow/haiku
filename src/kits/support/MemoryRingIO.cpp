/*
 * Copyright 2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Leorize, leorize+oss@disroot.org
 */


#include <MemoryRingIO.h>

#include <Autolock.h>

#include <stdlib.h>
#include <errno.h>


#define RING_MASK(x) ((x) & (fSize - 1))
#define TIMEOUT_QUANTA 100000


static size_t
next_power_of_two(size_t value)
{
	value--;
	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;
#if SIZE_MAX >= INT64_MAX
	value |= value >> 32;
#endif
	value++;

	return value;
}


BMemoryRingIO::BMemoryRingIO(size_t size)
	:
	fLocker("BMemoryRingIO"),
	fBuffer(NULL),
	fSize(0),
	fWrite(0),
	fRead(0),
	fFilled(false)
{
	SetSize(size);
}


BMemoryRingIO::~BMemoryRingIO()
{
	SetSize(0);
}


status_t
BMemoryRingIO::InitCheck()
{
	status_t result = fLocker.InitCheck();
	if (result != B_OK)
		return result;

	if (fSize == 0)
		return B_NO_INIT;

	return B_OK;
}


ssize_t
BMemoryRingIO::Read(void* _buffer, size_t size)
{
	if (_buffer == NULL)
		return B_BAD_VALUE;
	if (size == 0 || fBuffer == NULL)
		return 0;

	BAutolock _(fLocker);

	char* buffer = reinterpret_cast<char*>(_buffer);
	size_t bufPos = 0;
	ssize_t result = 0;
	while ((fRead != fWrite || fFilled) && bufPos < size) {
		buffer[bufPos] = fBuffer[fRead];

		bufPos++;
		fRead = RING_MASK(fRead + 1);
		result++;
		fFilled = false;
	}

	return result;
}


ssize_t
BMemoryRingIO::Write(const void* _buffer, size_t size)
{
	if (_buffer == NULL)
		return B_BAD_VALUE;
	if (size == 0 || fBuffer == NULL)
		return 0;

	BAutolock _(fLocker);

	const char* buffer = reinterpret_cast<const char*>(_buffer);
	size_t bufPos = 0;
	ssize_t result = 0;
	while ((fWrite != fRead || !fFilled) && bufPos < size) {
		fBuffer[fWrite] = buffer[bufPos];

		bufPos++;
		fWrite = RING_MASK(fWrite + 1);
		result++;
		fFilled = fRead == fWrite;
	}

	return result;
}


status_t
BMemoryRingIO::SetSize(size_t _size)
{
	BAutolock _(fLocker);

	const size_t size = next_power_of_two(_size);

	const size_t availableBytes = BytesAvailable();
	if (size < availableBytes)
		return B_NO_MEMORY;
	if (size == 0) {
		free(fBuffer);
		fBuffer = NULL;
		fSize = 0;
		Clear(); /* resets other internal counters */
		return B_OK;
	}

	char* newBuffer = reinterpret_cast<char*>(malloc(size));
	if (newBuffer == NULL) {
		return errno;
	}

	Read(newBuffer, availableBytes);
	free(fBuffer);

	fBuffer = newBuffer;
	fSize = size;
	fRead = 0;
	fWrite = RING_MASK(availableBytes);
	fFilled = fSize == availableBytes;

	return B_OK;
}


void
BMemoryRingIO::Clear()
{
	BAutolock _(fLocker);

	fRead = 0;
	fWrite = 0;
	fFilled = false;
}


size_t
BMemoryRingIO::BytesAvailable()
{
	BAutolock _(fLocker);

	return fWrite == fRead ? (fFilled ? fSize : 0) : RING_MASK(fWrite - fRead);
}


size_t
BMemoryRingIO::SpaceAvailable()
{
	BAutolock _(fLocker);

	return fWrite == fRead ? (fFilled ? 0 : fSize) : fSize - RING_MASK(fWrite - fRead);
}


size_t
BMemoryRingIO::GetSize()
{
	BAutolock _(fLocker);

	return fSize;
}


// Common implementation for WaitFor functions
#define WAIT_FOR(x, timeout) \
	const bool infinite = timeout == B_INFINITE_TIMEOUT; \
	const bigtime_t target = infinite ? 0 : system_time() + timeout; \
	\
	while (x) { \
		const bigtime_t current = system_time(); \
		if (!infinite && current >= target) \
			return B_TIMED_OUT; \
		const bigtime_t snoozeDuration = infinite ? TIMEOUT_QUANTA : \
			MIN(TIMEOUT_QUANTA, target - current); \
		status_t result = snooze(snoozeDuration); \
		if (result != B_OK) \
			return result; \
	} \
	\
	return B_OK;


status_t
BMemoryRingIO::WaitForRead(bigtime_t timeout)
{
	WAIT_FOR(BytesAvailable() == 0, timeout);
}


status_t
BMemoryRingIO::WaitForWrite(bigtime_t timeout)
{
	WAIT_FOR(SpaceAvailable() == 0, timeout);
}
