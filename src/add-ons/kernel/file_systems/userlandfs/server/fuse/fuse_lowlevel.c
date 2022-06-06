/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU LGPLv2.
  See the file COPYING.LIB
*/

#define _GNU_SOURCE

#include "config.h"
#include "fuse_api.h"
#include "fuse_i.h"
#include "fuse_kernel.h"
#include "fuse_opt.h"
#include "fuse_misc.h"
#include "fuse_common_compat.h"
#include "fuse_lowlevel_compat.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>
#include <sys/file.h>

static void convert_stat(const struct stat *stbuf, struct fuse_attr *attr)
{
	attr->ino	= stbuf->st_ino;
	attr->mode	= stbuf->st_mode;
	attr->nlink	= stbuf->st_nlink;
	attr->uid	= stbuf->st_uid;
	attr->gid	= stbuf->st_gid;
	attr->rdev	= stbuf->st_rdev;
	attr->size	= stbuf->st_size;
	attr->blksize	= stbuf->st_blksize;
	attr->blocks	= stbuf->st_blocks;
	attr->atime	= stbuf->st_atime;
	attr->mtime	= stbuf->st_mtime;
	attr->ctime	= stbuf->st_ctime;
	attr->atimensec = ST_ATIM_NSEC(stbuf);
	attr->mtimensec = ST_MTIM_NSEC(stbuf);
	attr->ctimensec = ST_CTIM_NSEC(stbuf);
}

size_t fuse_dirent_size(size_t namelen)
{
	return FUSE_DIRENT_ALIGN(FUSE_NAME_OFFSET + namelen);
}

char *fuse_add_dirent(char *buf, const char *name, const struct stat *stbuf,
		      off_t off)
{
	unsigned namelen = strlen(name);
	unsigned entlen = FUSE_NAME_OFFSET + namelen;
	unsigned entsize = fuse_dirent_size(namelen);
	unsigned padlen = entsize - entlen;
	struct fuse_dirent *dirent = (struct fuse_dirent *) buf;

	dirent->ino = stbuf->st_ino;
	dirent->off = off;
	dirent->namelen = namelen;
	dirent->type = (stbuf->st_mode & 0170000) >> 12;
	strncpy(dirent->name, name, namelen);
	if (padlen)
		memset(buf + entlen, 0, padlen);

	return buf + entsize;
}

size_t fuse_add_direntry(fuse_req_t req, char *buf, size_t bufsize,
			 const char *name, const struct stat *stbuf, off_t off)
{
	size_t entsize;

	(void) req;
	entsize = fuse_dirent_size(strlen(name));
	if (entsize <= bufsize && buf)
		fuse_add_dirent(buf, name, stbuf, off);
	return entsize;
}

static int send_reply_ok(fuse_req_t req, const void *arg, size_t argsize)
{
	return -1;
}

static unsigned long calc_timeout_sec(double t)
{
	if (t > (double) ULONG_MAX)
		return ULONG_MAX;
	else if (t < 0.0)
		return 0;
	else
		return (unsigned long) t;
}

static unsigned int calc_timeout_nsec(double t)
{
	double f = t - (double) calc_timeout_sec(t);
	if (f < 0.0)
		return 0;
	else if (f >= 0.999999999)
		return 999999999;
	else
		return (unsigned int) (f * 1.0e9);
}

static void fill_entry(struct fuse_entry_out *arg,
		       const struct fuse_entry_param *e)
{
	arg->nodeid = e->ino;
	arg->generation = e->generation;
	arg->entry_valid = calc_timeout_sec(e->entry_timeout);
	arg->entry_valid_nsec = calc_timeout_nsec(e->entry_timeout);
	arg->attr_valid = calc_timeout_sec(e->attr_timeout);
	arg->attr_valid_nsec = calc_timeout_nsec(e->attr_timeout);
	convert_stat(&e->attr, &arg->attr);
}

static void fill_open(struct fuse_open_out *arg,
		      const struct fuse_file_info *f)
{
	arg->fh = f->fh;
	if (f->direct_io)
		arg->open_flags |= FOPEN_DIRECT_IO;
	if (f->keep_cache)
		arg->open_flags |= FOPEN_KEEP_CACHE;
	if (f->nonseekable)
		arg->open_flags |= FOPEN_NONSEEKABLE;
}

int fuse_reply_create(fuse_req_t req, const struct fuse_entry_param *e,
		      const struct fuse_file_info *f)
{
	char buf[sizeof(struct fuse_entry_out) + sizeof(struct fuse_open_out)];
	size_t entrysize = req->f->conn.proto_minor < 9 ?
		FUSE_COMPAT_ENTRY_OUT_SIZE : sizeof(struct fuse_entry_out);
	struct fuse_entry_out *earg = (struct fuse_entry_out *) buf;
	struct fuse_open_out *oarg = (struct fuse_open_out *) (buf + entrysize);

	memset(buf, 0, sizeof(buf));
	fill_entry(earg, e);
	fill_open(oarg, f);
	return send_reply_ok(req, buf,
			     entrysize + sizeof(struct fuse_open_out));
}

int fuse_reply_write(fuse_req_t req, size_t count)
{
	struct fuse_write_out arg;

	memset(&arg, 0, sizeof(arg));
	arg.size = count;

	return send_reply_ok(req, &arg, sizeof(arg));
}
