/*
 * Copyright 2024 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <devctl.h>
#include <syscalls.h>
#include <errno.h>

#include <errno_private.h>
#include <syscall_utils.h>


int
posix_devctl(int fd, int cmd, void* argument, size_t size, int* result)
{
	int status = _kern_ioctl(fd, cmd, argument, size);

	if (status < B_OK)
		return status;

	*result = status;
	return B_OK;
}
