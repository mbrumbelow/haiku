/*
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <OS.h>
#include <malloc.h>
#include <sys/param.h>

#include <syscalls.h>
#include <user_mutex_defs.h>
#include <errno_private.h>
#include <libroot_private.h>


/* generic stuff */
#if B_PAGE_SIZE == 4096
#define _MAX_PAGE_SHIFT 12
#endif

extern char* __progname;

#define GET_THREAD_ID()		find_thread(NULL)

#define MAP_CONCEAL		(0)
#define __MAP_NOREPLACE		(0)

#define DEF_STRONG(X)
#define	DEF_WEAK(x)


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
malloc_arc4random_buf(void *buf, size_t nbytes)
{
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
mimmutable(void *, size_t)
{
	/* do nothing */
	return 0;
}


static void*
malloc_mmap(void* address, size_t length, int protection, int flags, int fd, off_t offset)
{
	uint32 addressSpec;
	if ((flags & MAP_FIXED) != 0) {
		addressSpec = B_EXACT_ADDRESS;
	} else if (address != NULL) {
		addressSpec = B_BASE_ADDRESS;
	} else {
#if 1
		addressSpec = B_ANY_ADDRESS;
#else
		// TODO: Fix ASLR to not waste so much address space.
		addressSpec = B_RANDOMIZED_ANY_ADDRESS;
#endif
	}

	uint32 areaProtection = B_READ_AREA | B_WRITE_AREA;
	area_id area = create_area("heap area", &address, addressSpec,
		length, 0, areaProtection);
	if (area < 0) {
		__set_errno(area);
		return MAP_FAILED;
	}

	return address;
}
#define mmap malloc_mmap


/* malloc-specific */

#define _MALLOC_MUTEXES 32


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


static inline void
malloc_lock(int32* lock)
{
	int32 oldValue = atomic_test_and_set(lock, B_USER_MUTEX_LOCKED, 0);
	if (oldValue == 0)
		return;

	status_t error;
	do {
		error = _kern_mutex_lock(lock, "heap lock", 0, 0);
	} while (error == B_INTERRUPTED);

	if (error != B_OK)
		debugger("malloc_lock() failed");
}
#define _MALLOC_LOCK(LOCK) malloc_lock(&LOCK)


static inline void
malloc_unlock(int32* lock)
{
	int32 oldValue = atomic_and(lock, ~(int32)B_USER_MUTEX_LOCKED);
	if ((oldValue & B_USER_MUTEX_WAITING) != 0)
		_kern_mutex_unblock(lock, 0);

	if ((oldValue & B_USER_MUTEX_LOCKED) == 0)
		debugger("mutex was not actually locked!");
}
#define _MALLOC_UNLOCK(LOCK) malloc_unlock(&LOCK)


static void _malloc_init(int from_rthreads);


/* Haiku internal APIs */

status_t
__init_heap()
{
	_malloc_init(1);
	return B_OK;
}


void
__heap_terminate_after()
{
}


void
__heap_before_fork()
{
}


void
__heap_after_fork_child()
{
}


void
__heap_after_fork_parent()
{
}


void
__heap_thread_init()
{
}


void
__heap_thread_exit()
{
}

