/*
 * Copyright 2020, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "glue.h"

#include <libroot/errno_private.h>


void*
__malloc_mmap(void* address, size_t length, int protection, int flags,
	int _fd, off_t _offset)
{
	if ((flags & MAP_ANONYMOUS) == 0 || (flags & MAP_PRIVATE) == 0) {
		__set_errno(EBADF);
		return MAP_FAILED;
	}

	uint32 addressSpec;
	if ((flags & MAP_FIXED) != 0)
		addressSpec = B_EXACT_ADDRESS;
	else if (address != NULL)
		addressSpec = B_BASE_ADDRESS;
	else
		addressSpec = B_ANY_ADDRESS;

	uint32 areaProtection = 0;
	if ((protection & PROT_READ) != 0)
		areaProtection |= B_READ_AREA;
	if ((protection & PROT_WRITE) != 0)
		areaProtection |= B_WRITE_AREA;
	if ((protection & PROT_EXEC) != 0)
		areaProtection |= B_EXECUTE_AREA;

	area_id area = create_area("heap area", &address, addressSpec,
		length, B_NO_LOCK, areaProtection);
	if (area < 0) {
		__set_errno(area);
		return MAP_FAILED;
	}

	return address;
}


//	#pragma mark - libroot hooks


void
__init_heap(void)
{
}


void
__heap_terminate_after(void)
{
}


void
__heap_before_fork(void)
{
}


void
__heap_after_fork_child(void)
{
}


void
__heap_after_fork_parent(void)
{
}


void
__heap_thread_init(void)
{
}


void
__heap_thread_exit(void)
{
}


//	#pragma mark - BeOS specific extensions


#if __GNUC__ == 2

struct mstats {
	size_t bytes_total;
	size_t chunks_used;
	size_t bytes_used;
	size_t chunks_free;
	size_t bytes_free;
};


extern "C" struct mstats
mstats(void)
{
	// TODO?
	struct mstats stats;
	return stats;
}

#endif
