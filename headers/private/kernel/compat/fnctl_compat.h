/*
 * Copyright 2018, Haiku Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_COMPAT_FNCTL_H
#define _KERNEL_COMPAT_FNCTL_H


#include <fnctl.h>


struct compat_flock {
	short	l_type;
	short	l_whence;
	off_t	l_start;
	off_t	l_len;
	pid_t	l_pid;
} _PACKED;


#endif // _KERNEL_COMPAT_FNCTL_H
