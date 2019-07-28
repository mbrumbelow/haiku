/*
 * Copyright 2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */


#include <OS.h>
#include <Locker.h>
#include <pthread.h>
#include <libroot/pthread_private.h>

#include <stdio.h>


BLocker::BLocker()
{
	InitLocker(NULL, true);
}


BLocker::BLocker(const char *name)
{
	InitLocker(name, true);
}


BLocker::BLocker(bool benaphoreStyle)
{
	InitLocker(NULL, benaphoreStyle);
}


BLocker::BLocker(const char *name, bool benaphoreStyle)
{
	InitLocker(name, benaphoreStyle);
}


BLocker::~BLocker()
{
	if (fSemaphoreID >= 0)
		delete_sem(fSemaphoreID);
}


status_t
BLocker::InitCheck() const
{
	return fMutex.owner_count >= 0 ? B_OK : fMutex.owner_count;
}


bool
BLocker::Lock()
{
	status_t result;
	return AcquireLock(B_INFINITE_TIMEOUT, &result);
}


status_t
BLocker::LockWithTimeout(bigtime_t timeout)
{
	status_t result;

	AcquireLock(timeout, &result);
	return result;
}


void
BLocker::Unlock()
{
	// The Be Book explicitly allows any thread, not just the lock owner, to
	// unlock. This is bad practice, but we must allow it for compatibility
	// reasons. We can at least warn the developer that something is probably
	// wrong.
	if (!IsLocked()) {
		fprintf(stderr, "Unlocking BLocker with sem %" B_PRId32
			" from wrong thread %" B_PRId32 ", current holder %" B_PRId32 "\n",
			fSemaphoreID, find_thread(NULL), fMutex.owner);
		fMutex.owner = find_thread(NULL);
	}

	pthread_mutex_unlock(&fMutex);

	atomic_add(&fMutex.unused, -1);

	if (fMutex.owner_count == 0 && fSemaphoreID >= 0)
		release_sem(fSemaphoreID);
}


thread_id
BLocker::LockingThread() const
{
	return fMutex.owner;
}


bool
BLocker::IsLocked() const
{
	return find_thread(NULL) == LockingThread();
}


int32
BLocker::CountLocks() const
{
	return fMutex.owner_count;
}


int32
BLocker::CountLockRequests() const
{
	return atomic_get((int32*)&fMutex.unused);
}


sem_id
BLocker::Sem() const
{
	return fSemaphoreID;
}


void
BLocker::InitLocker(const char *name, bool benaphore)
{
	if (!benaphore) {
		if (name == NULL)
			name = "some BLocker";
		fSemaphoreID = create_sem(1, name);
		if (fSemaphoreID < B_OK) {
			fMutex.owner_count = fSemaphoreID;
			return;
		}
	} else
		fSemaphoreID = -1;

	pthread_mutexattr attr = {
		PTHREAD_MUTEX_RECURSIVE,
		false
	};
	pthread_mutexattr_t attrib = &attr;
	if (pthread_mutex_init(&fMutex, &attrib) != B_OK)
		fMutex.owner_count = B_ERROR;

	fMutex.unused = 0;
		// we are out of room in this class, so we use
		// this field to store the lock requests count
}


bool
BLocker::AcquireLock(bigtime_t timeout, status_t *error)
{
	status_t status = B_OK;

	atomic_add(&fMutex.unused, 1);

	if (fSemaphoreID >= 0 && !IsLocked()) {
		do {
			status = acquire_sem_etc(fSemaphoreID, 1, B_RELATIVE_TIMEOUT,
				timeout);
		} while (status == B_INTERRUPTED);

		if (status != B_OK) {
			atomic_add(&fMutex.unused, -1);
			*error = status;
			return false;
		}
	}

	status = __pthread_mutex_lock(&fMutex, timeout);
	*error = status;
	if (status != B_OK)
		atomic_add(&fMutex.unused, -1);
	return status == B_OK;
}


#ifdef _BEOS_R5_COMPATIBLE_
BLocker::BLocker(const char *name, bool benaphoreStyle,
	bool /* for_IPC */)
{
	InitLocker(name, benaphoreStyle);
}
#endif
