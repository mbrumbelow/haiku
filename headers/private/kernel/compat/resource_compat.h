/*
 * Copyright 2018, Haiku Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_COMPAT_RESOURCE_H
#define _KERNEL_COMPAT_RESOURCE_H


#include <sys/resource.h>


typedef uint32 compat_rlim_t;
struct compat_rlimit {
	compat_rlim_t	rlim_cur;		/* current soft limit */
	compat_rlim_t	rlim_max;		/* hard limit */
};


#endif // _KERNEL_COMPAT_RESOURCE_H
