/*
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <OS.h>
#include <malloc.h>
#include <pthread.h>
#include <sys/param.h>

#include <errno_private.h>
#include <libroot_private.h>
#include <shared/locks.h>
#include <system/tls.h>


/* generic stuff */
#if B_PAGE_SIZE == 4096
#define _MAX_PAGE_SHIFT 12
#endif

extern char* __progname;

#define MAP_CONCEAL		(0)
#define __MAP_NOREPLACE		(0)

#define DEF_STRONG(X)
#define	DEF_WEAK(x)

static int32
get_cpu_count()
{
	system_info info;
	if (get_system_info(&info) != B_OK)
		return 1;
	return info.cpu_count;
}


/* entropy routines, wrapped for malloc */

static uint32_t
malloc_arc4random()
{
	// TODO: Improve this?
	const uintptr_t address = (uintptr_t)&address;
	uint32_t random = (uint32_t)address * 1103515245;
	random += (uint32_t)system_time();
	return random;
}
#define arc4random malloc_arc4random


static void
malloc_arc4random_buf(void *_buf, size_t nbytes)
{
	uint8* buf = (uint8*)_buf;
	while (nbytes > 0) {
		uint32_t value = malloc_arc4random();
		const size_t copy = (nbytes > 4) ? 4 : nbytes;
		memcpy(buf, &value, copy);
		nbytes -= copy;
		buf += copy;
	}
}
#define arc4random_buf malloc_arc4random_buf


static uint32_t
malloc_arc4random_uniform(uint32_t upper_bound)
{
	return (malloc_arc4random() % upper_bound);
}
#define arc4random_uniform malloc_arc4random_uniform


/* memory routines, wrapped for malloc */

static inline void
malloc_explicit_bzero(void* buf, size_t len)
{
	memset(buf, 0, len);
}
#define explicit_bzero malloc_explicit_bzero


static int
malloc_mprotect(void* address, size_t length, int protection)
{
	/* do nothing */
	return 0;
}
#define mprotect malloc_mprotect


static int
mimmutable(void*, size_t)
{
	/* do nothing */
	return 0;
}


/* memory mapping */

static area_id sLastArea = -1;
static addr_t sLastAreaTop = 0;
static size_t sLastAreaSize = 0;
static mutex sLastAreaLock;


static void*
malloc_mmap(void* address, size_t length, int protection, int flags, int fd, off_t offset)
{
	uint32 addressSpec;
	if ((flags & MAP_FIXED) != 0) {
		addressSpec = B_EXACT_ADDRESS;
	} else if (address != NULL) {
		addressSpec = B_BASE_ADDRESS;
	} else {
		addressSpec = B_ANY_ADDRESS;
	}

	// Try to resize the last area rather than creating a new one.
	do {
		mutex_lock(&sLastAreaLock);
		if (sLastAreaSize == 0) {
			mutex_unlock(&sLastAreaLock);
			break;
		}
		if (addressSpec == B_EXACT_ADDRESS && sLastAreaTop != (addr_t)address) {
			if (address >= (sLastAreaTop - sLastAreaSize) && address < sLastAreaTop) {
				// That's not going to work.
				mutex_unlock(&sLastAreaLock);
				__set_errno(EEXIST);
				return MAP_FAILED;
			}
			mutex_unlock(&sLastAreaLock);
			break;
		}

		status_t status = resize_area(sLastArea, sLastAreaSize + length);
		if (status != B_OK) {
			mutex_unlock(&sLastAreaLock);
			break;
		}

		const addr_t address = sLastAreaTop;
		sLastAreaTop += length;
		sLastAreaSize += length;
		mutex_unlock(&sLastAreaLock);

		return (void*)address;
	} while (false);

	uint32 areaProtection = B_READ_AREA | B_WRITE_AREA;
	area_id area = create_area("heap area", &address, addressSpec,
		length, 0, areaProtection);
	if (area < 0) {
		__set_errno(area);
		return MAP_FAILED;
	}

	if (addressSpec != B_EXACT_ADDRESS) {
		mutex_lock(&sLastAreaLock);
		sLastArea = area;
		sLastAreaTop = (addr_t)address + length;
		sLastAreaSize = length;
		mutex_unlock(&sLastAreaLock);
	}

	return address;
}
#define mmap malloc_mmap


static int
malloc_munmap(void* address, size_t length)
{
	const addr_t unmapAddress = (addr_t)address;
	const addr_t unmapTop = unmapAddress + length;

	mutex_lock(&sLastAreaLock);
	const addr_t lastAreaTop = sLastAreaTop;
	const addr_t lastAreaBase = (lastAreaTop - sLastAreaSize);
	if (unmapAddress <= lastAreaBase && unmapTop >= lastAreaTop) {
		// The whole area is being deleted.
		const area_id lastArea = sLastArea;
		sLastArea = -1;
		sLastAreaTop = sLastAreaSize = 0;
		mutex_unlock(&sLastAreaLock);

		if (unmapAddress == lastAreaBase && unmapTop == lastAreaTop) {
			delete_area(lastArea);
			return 0;
		}
		return munmap(address, length);
	} else if (unmapTop == lastAreaTop) {
		// Shrink the top.
		status_t status = resize_area(sLastArea, sLastAreaSize - length);
		if (status != B_OK) {
			mutex_unlock(&sLastAreaLock);
			__set_errno(status);
			return -1;
		}
		sLastAreaSize -= length;
		sLastAreaTop -= length;
		mutex_unlock(&sLastAreaLock);
		return 0;
	} else if (unmapAddress == lastAreaBase) {
		// Shrink the bottom.
		sLastAreaSize -= length;
		mutex_unlock(&sLastAreaLock);
		return munmap(address, length);
	} else if (unmapAddress >= lastAreaBase && unmapAddress < lastAreaTop) {
		// Cut the middle and get the new ID.
		if (munmap(address, length) != 0) {
			mutex_unlock(&sLastAreaLock);
			return -1;
		}

		sLastAreaSize = lastAreaTop - unmapTop;
		sLastArea = area_for((void*)unmapTop);
		mutex_unlock(&sLastAreaLock);
		return 0;
	}

	// Not in the last area; call regular unmap.
	mutex_unlock(&sLastAreaLock);
	return munmap(address, length);
}
#define munmap malloc_munmap


/* public methods */

void*
memalign(size_t align, size_t len)
{
	void* result = NULL;
	int status;

	if (align < sizeof(void*))
		align = sizeof(void*);

	status = posix_memalign(&result, align, len);
	if (status != 0) {
		errno = status;
		return NULL;
	}
	return result;
}


/* malloc implementation */

#define _MALLOC_MUTEXES 32
static mutex sMallocMutexes[_MALLOC_MUTEXES];
static pthread_once_t sThreadedMallocInitOnce = PTHREAD_ONCE_INIT;

static int32 sNextMallocThreadID = 1;

static u_int mopts_nmutexes();
static void _malloc_init(int from_rthreads);


static inline void
_MALLOC_LOCK(int32 index)
{
	mutex_lock(&sMallocMutexes[index]);
}


static inline void
_MALLOC_UNLOCK(int32 index)
{
	mutex_unlock(&sMallocMutexes[index]);
}


static void
init_threaded_malloc()
{
	for (u_int i = 2; i < _MALLOC_MUTEXES; i++)
		mutex_init(&sMallocMutexes[i], "heap mutex");

	_MALLOC_LOCK(0);
	_malloc_init(1);
	_MALLOC_UNLOCK(0);
}


status_t
__init_heap()
{
	tls_set(TLS_MALLOC_SLOT, (void*)0);
	mutex_init(&sLastAreaLock, "heap last area");
	mutex_init(&sMallocMutexes[0], "heap mutex");
	mutex_init(&sMallocMutexes[1], "heap mutex");
	_malloc_init(0);
	return B_OK;
}


static int32
get_thread_malloc_id()
{
	int32 result = (int32)(intptr_t)tls_get(TLS_MALLOC_SLOT);
	if (result == -1) {
		// thread has never called malloc() before; assign it an ID.
		result = atomic_add(&sNextMallocThreadID, 1);
		tls_set(TLS_MALLOC_SLOT, (void*)(intptr_t)result);
	}
	return result;
}


void
__heap_thread_init()
{
	pthread_once(&sThreadedMallocInitOnce, &init_threaded_malloc);
	tls_set(TLS_MALLOC_SLOT, (void*)(intptr_t)-1);
}


void
__heap_thread_exit()
{
	const int32 id = (int32)(intptr_t)tls_get(TLS_MALLOC_SLOT);
	if (id != -1 && id == (sNextMallocThreadID - 1)) {
		// Try to "de-allocate" this thread's ID.
		atomic_test_and_set(&sNextMallocThreadID, id, id + 1);
	}
}


void
__heap_before_fork()
{
	u_int nmutexes = mopts_nmutexes();
	for (u_int i = 0; i < nmutexes; i++)
		_MALLOC_LOCK(i);

	mutex_lock(&sLastAreaLock);
}


void
__heap_after_fork_child()
{
	u_int nmutexes = mopts_nmutexes();
	for (u_int i = 0; i < nmutexes; i++)
		mutex_init(&sMallocMutexes[i], "heap mutex");

	sLastArea = area_for((void*)(sLastAreaTop - sLastAreaSize));
	mutex_init(&sLastAreaLock, "heap mutex");
}


void
__heap_after_fork_parent()
{
	u_int nmutexes = mopts_nmutexes();
	for (u_int i = 0; i < nmutexes; i++)
		_MALLOC_UNLOCK(i);

	mutex_unlock(&sLastAreaLock);
}


void
__heap_terminate_after()
{
}
