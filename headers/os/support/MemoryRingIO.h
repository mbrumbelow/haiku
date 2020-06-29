/*
 * Copyright 2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MEMORY_RING_IO_H
#define _MEMORY_RING_IO_H


#include <DataIO.h>
#include <Locker.h>


class BMemoryRingIO : public BDataIO {
public:
								BMemoryRingIO(size_t size);
	virtual						~BMemoryRingIO();

			status_t			InitCheck();

	virtual	ssize_t				Read(void* buffer, size_t size);
	virtual	ssize_t				Write(const void* buffer, size_t size);

			status_t			SetSize(size_t size);
			void				Clear();

			size_t				BytesAvailable();
			size_t				SpaceAvailable();
			size_t				GetSize();

			status_t			WaitForRead(bigtime_t timeout = B_INFINITE_TIMEOUT);
			status_t			WaitForWrite(bigtime_t timeout = B_INFINITE_TIMEOUT);
private:
			BLocker				fLocker;

			char*				fBuffer;

			size_t				fSize; // Total buffer size
			size_t				fWrite; // Next position to write to
			size_t				fRead; // Next position to read from

			bool				fFilled; // Whether the entire buffer is filled
};


#endif	// _MEMORY_RING_IO_H
