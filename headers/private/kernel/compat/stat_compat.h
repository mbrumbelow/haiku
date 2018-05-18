/*
 * Copyright 2018, Haiku Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_COMPAT_STAT_H
#define _KERNEL_COMPAT_STAT_H


#include <sys/stat.h>
#include <time_compat.h>


struct compat_stat {
	dev_t			st_dev;			/* device ID that this file resides on */
	ino_t			st_ino;			/* this file's serial inode ID */
	mode_t			st_mode;		/* file mode (rwx for user, group, etc) */
	nlink_t			st_nlink;		/* number of hard links to this file */
	uid_t			st_uid;			/* user ID of the owner of this file */
	gid_t			st_gid;			/* group ID of the owner of this file */
	off_t			st_size;		/* size in bytes of this file */
	dev_t			st_rdev;		/* device type (not used) */
	blksize_t		st_blksize;		/* preferred block size for I/O */
	struct compat_timespec	st_atim;		/* last access time */
	struct compat_timespec	st_mtim;		/* last modification time */
	struct compat_timespec	st_ctim;		/* last change time, not creation time */
	struct compat_timespec	st_crtim;		/* creation time */
	__haiku_uint32	st_type;		/* attribute/index type */
	blkcnt_t		st_blocks;		/* number of blocks allocated for object */
} _PACKED;


#endif // _KERNEL_COMPAT_STAT_H
