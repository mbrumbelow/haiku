/*
 * Copyright 2018, Haiku Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_COMPAT_FS_ATTR_H
#define _KERNEL_COMPAT_FS_ATTR_H


#include <fs_attr.h>


struct compat_attr_info {
	uint32	type;
	off_t	size;
} _PACKED;


#endif // _KERNEL_COMPAT_FS_ATTR_H
