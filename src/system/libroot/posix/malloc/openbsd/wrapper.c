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
static pthread_mutex_t sMallocMutexes[_MALLOC_MUTEXES];
static pthread_once_t sThreadedMallocInitOnce = PTHREAD_ONCE_INIT;

static u_int mopts_nmutexes();
static void _malloc_init(int from_rthreads);


static inline void
_MALLOC_LOCK(int32 index)
{
	pthread_mutex_lock(&sMallocMutexes[index]);
}


static inline void
_MALLOC_UNLOCK(int32 index)
{
	pthread_mutex_unlock(&sMallocMutexes[index]);
}


static void
init_threaded_malloc()
{
	u_int i;
	for (i = 2; i < _MALLOC_MUTEXES; i++)
		pthread_mutex_init(&sMallocMutexes[i], NULL);

	_MALLOC_LOCK(0);
	_malloc_init(1);
	_MALLOC_UNLOCK(0);
}


status_t
__init_heap()
{
	pthread_mutex_init(&sMallocMutexes[0], NULL);
	pthread_mutex_init(&sMallocMutexes[1], NULL);
	_malloc_init(0);
	return B_OK;
}


void
__heap_terminate_after()
{
}


void
__heap_before_fork()
{
	u_int i;
	u_int nmutexes = mopts_nmutexes();
	for (i = 0; i < nmutexes; i++)
		_MALLOC_LOCK(i);
}


void
__heap_after_fork_child()
{
	u_int i;
	u_int nmutexes = mopts_nmutexes();
	for (i = 0; i < nmutexes; i++)
		pthread_mutex_init(&sMallocMutexes[i], NULL);
}


void
__heap_after_fork_parent()
{
	u_int i;
	u_int nmutexes = mopts_nmutexes();
	for (i = 0; i < nmutexes; i++)
		_MALLOC_UNLOCK(i);
}


void
__heap_thread_init()
{
	pthread_once(&sThreadedMallocInitOnce, &init_threaded_malloc);
}


void
__heap_thread_exit()
{
}
