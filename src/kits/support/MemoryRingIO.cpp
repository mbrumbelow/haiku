/*
 * Copyright 2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Leorize, leorize+oss@disroot.org
 */


#include <MemoryRingIO.h>

#include <AutoLocker.h>

#include <stdlib.h>
#include <string.h>


class PThreadLocking {
	public:
	inline bool Lock(pthread_mutex_t* mutex)
	{
		/* NOTE: We are relying on behavior specific to Haiku: the default
		 * mutex type have error checking for relock within the same thread.
		 */
		return pthread_mutex_lock(mutex) == 0;
	}

	inline void Unlock(pthread_mutex_t* mutex)
	{
		pthread_mutex_unlock(mutex);
	}
};


typedef AutoLocker<pthread_mutex_t, PThreadLocking> PThreadAutoLocker;


#define RING_MASK(x) ((x) & (fSize - 1))


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
	fLock(PTHREAD_MUTEX_INITIALIZER),
	fEvent(PTHREAD_COND_INITIALIZER),
	fBuffer(NULL),
	fSize(0),
	fWrite(0),
	fRead(0),
	fFilled(false),
	fEnded(false)
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

	PThreadAutoLocker _(fLock);

	if (!fEnded)
		WaitForRead();
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

	pthread_cond_signal(&fEvent);

	return result;
}


ssize_t
BMemoryRingIO::Write(const void* _buffer, size_t size)
{
	if (_buffer == NULL)
		return B_BAD_VALUE;
	if (size == 0 || fBuffer == NULL)
		return 0;

	PThreadAutoLocker _(fLock);

	if (!fEnded)
		WaitForWrite();
	// We separate this check from WaitForWrite() as the boolean
	// might have been toggled during our wait on the conditional.
	if (fEnded)
		return B_DEVICE_FULL;
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

	pthread_cond_signal(&fEvent);

	return result;
}


status_t
BMemoryRingIO::SetSize(size_t _size)
{
	PThreadAutoLocker _(fLock);

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
		return B_NO_MEMORY;
	}

	Read(newBuffer, availableBytes);
	free(fBuffer);

	fBuffer = newBuffer;
	fSize = size;
	fRead = 0;
	fWrite = RING_MASK(availableBytes);
	fFilled = fSize == availableBytes;

	pthread_cond_signal(&fEvent);

	return B_OK;
}


void
BMemoryRingIO::Clear()
{
	PThreadAutoLocker _(fLock);

	fRead = 0;
	fWrite = 0;
	fFilled = false;
}


size_t
BMemoryRingIO::BytesAvailable()
{
	PThreadAutoLocker _(fLock);

	if (fWrite == fRead) {
		if (fFilled)
			return fSize;
		return 0;
	}
	return RING_MASK(fWrite - fRead);
}


size_t
BMemoryRingIO::SpaceAvailable()
{
	PThreadAutoLocker _(fLock);

	if (fEnded)
		return 0;
	if (fWrite == fRead) {
		if (fFilled)
			return 0;
		return fSize;
	}
	return RING_MASK(fWrite - fRead);
}


size_t
BMemoryRingIO::GetSize()
{
	PThreadAutoLocker _(fLock);

	return fSize;
}


static status_t
_CondWait(pthread_cond_t& cond, pthread_mutex_t& lock, bigtime_t timeout)
{
	if (timeout == B_INFINITE_TIMEOUT) {
		pthread_cond_wait(&cond, &lock);
	} else {
		bigtime_t target = system_time() + timeout;
		struct timespec absTimeout;
		memset(&absTimeout, 0, sizeof(absTimeout));
		absTimeout.tv_sec = target / 100000;
		absTimeout.tv_nsec = (target % 100000) * 1000L;

		int err = pthread_cond_timedwait(&cond, &lock, &absTimeout);
		if (err != B_OK)
			return err;
	}

	return B_OK;
}


status_t
BMemoryRingIO::WaitForRead(bigtime_t timeout)
{
	PThreadAutoLocker autoLocker(fLock);

	while (BytesAvailable() == 0) {
		if (fEnded)
			return B_WOULD_BLOCK;
		status_t result = _CondWait(fEvent, fLock, timeout);
		if (result != B_OK)
			return result;
	}

	return B_OK;
}


status_t
BMemoryRingIO::WaitForWrite(bigtime_t timeout)
{
	PThreadAutoLocker autoLocker(fLock);

	while (SpaceAvailable() == 0) {
		if (fEnded)
			return B_WOULD_BLOCK;
		status_t result = _CondWait(fEvent, fLock, timeout);
		if (result != B_OK)
			return result;
	}

	return B_OK;
}


void
BMemoryRingIO::SetEndReached(bool end)
{
	PThreadAutoLocker autoLocker(fLock);

	fEnded = end;

	pthread_cond_signal(&fEvent);
}
