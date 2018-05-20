/*
 * Copyright 2018, Haiku Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_COMPAT_THREAD_DEFS_H
#define _KERNEL_COMPAT_THREAD_DEFS_H


#include <thread_defs.h>


#define compat_ptr_t uint32
struct compat_thread_creation_attributes {
	compat_ptr_t	entry;
	compat_ptr_t	name;
	int32		priority;
	compat_ptr_t	args1;
	compat_ptr_t	args2;
	compat_ptr_t	stack_address;
	uint32_t	stack_size;
	uint32_t	guard_size;
	compat_ptr_t	pthread;
	uint32		flags;
} _PACKED;


// STATIC_ASSERT(sizeof(compatAttrs) == 40);


#endif // _KERNEL_COMPAT_THREAD_DEFS_H
