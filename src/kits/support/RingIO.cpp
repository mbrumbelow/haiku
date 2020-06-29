/*
 * Copyright 2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Leorize, leorize+oss@disroot.org
 */


#include <RingIO.h>

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


BRingIO::BRingIO(size_t size)
	:
	fLocker("BRingIO"),
	fBuffer(NULL),
	fSize(0),
	fWrite(0),
	fRead(0),
	fFilled(false)
{
	SetSize(size);
}


BRingIO::~BRingIO()
{
	SetSize(0);
}


status_t
BRingIO::InitCheck()
{
	status_t result = fLocker.InitCheck();
	if (result != B_OK)
		return result;

	if (fSize == 0)
		return B_NO_INIT;

	return B_OK;
}


ssize_t
BRingIO::Read(void* _buffer, size_t size)
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
BRingIO::Write(const void* _buffer, size_t size)
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
BRingIO::SetSize(size_t _size)
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
BRingIO::Clear()
{
	BAutolock _(fLocker);

	fRead = 0;
	fWrite = 0;
	fFilled = false;
}


size_t
BRingIO::BytesAvailable()
{
	BAutolock _(fLocker);

	return fWrite == fRead ? (fFilled ? fSize : 0) : RING_MASK(fWrite - fRead);
}


size_t
BRingIO::SpaceAvailable()
{
	BAutolock _(fLocker);

	return fWrite == fRead ? (fFilled ? 0 : fSize) : fSize - RING_MASK(fWrite - fRead);
}


size_t
BRingIO::GetSize()
{
	BAutolock _(fLocker);

	return fSize;
}


status_t
BRingIO::WaitForRead(bigtime_t timeout)
{
	bigtime_t target = system_time() + timeout;

	while (BytesAvailable() == 0) {
		bigtime_t current = system_time();
		if (timeout != B_INFINITE_TIMEOUT && current >= target)
			return B_TIMED_OUT;
		status_t result = snooze(MIN(TIMEOUT_QUANTA, target - current));
		if (result != B_OK)
			return result;
	}

	return B_OK;
}


status_t
BRingIO::WaitForWrite(bigtime_t timeout)
{
	bigtime_t target = system_time() + timeout;

	while (SpaceAvailable() == 0) {
		bigtime_t current = system_time();
		if (timeout != B_INFINITE_TIMEOUT && current >= target)
			return B_TIMED_OUT;
		status_t result = snooze(MIN(TIMEOUT_QUANTA, target - current));
		if (result != B_OK)
			return result;
	}

	return B_OK;
}
