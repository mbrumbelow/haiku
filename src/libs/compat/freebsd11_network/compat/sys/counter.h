/*
 * Copyright 2017-2018, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_COUNTER_H_
#define _FBSD_COMPAT_SYS_COUNTER_H_

#include <machine/atomic.h>
#include <sys/malloc.h>


typedef uint64_t* counter_u64_t;


static inline counter_u64_t
counter_u64_alloc(int wait)
{
	return (counter_u64_t)_kernel_malloc(sizeof(uint64_t), wait | M_ZERO);
}


static inline void
counter_u64_free(counter_u64_t c)
{
	_kernel_free(c);
}


static inline void
counter_u64_add(counter_u64_t c, int64_t v)
{
	atomic_add64((int64*)c, v);
}


static inline uint64_t
counter_u64_fetch(counter_u64_t c)
{
	return atomic_get64((int64*)c);
}


static inline void
counter_u64_zero(counter_u64_t c)
{
	atomic_set64((int64*)c, 0);
}


static inline void
counter_enter()
{
	// unneeded; counters are atomic
}


static inline void
counter_exit()
{
	// unneeded; counters are atomic
}


static inline void
counter_u64_add_protected(counter_u64_t c, int64_t v)
{
	// counters are atomic
	counter_u64_add(c, v);
}


#endif
