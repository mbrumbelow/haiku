/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef FS_OPS_SUPPORT_H
#define FS_OPS_SUPPORT_H

#ifndef FS_SHELL
#	include <kernel.h>
#	include <dirent.h>
#else
#	include "fssh_kernel_priv.h"
#endif


static struct dirent*
next_dirent(struct dirent* dirent, size_t nameLength, size_t& bufferRemaining)
{
	const size_t reclen = offsetof(struct dirent, d_name) + nameLength + 1;
	ASSERT(reclen <= bufferRemaining);
	dirent->d_reclen = reclen;

	const size_t rounded_reclen = ROUNDUP(reclen, alignof(struct dirent));
	if (rounded_reclen >= bufferRemaining) {
		bufferRemaining -= reclen;
		return NULL;
	}
	dirent->d_reclen = rounded_reclen;
	bufferRemaining -= rounded_reclen;

	if (bufferRemaining < offsetof(struct dirent, d_name))
		return NULL;

	dirent = (struct dirent*)((uint8*)dirent + rounded_reclen);
	return dirent;
}


#endif	// FS_OPS_SUPPORT_H
