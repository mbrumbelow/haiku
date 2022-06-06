/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU LGPLv2.
  See the file COPYING.LIB
*/

#define _GNU_SOURCE

#include "config.h"
#include "fuse_api.h"
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
	memcpy(dirent->name, name, namelen);
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

int fuse_reply_create(fuse_req_t req, const struct fuse_entry_param *e,
		      const struct fuse_file_info *f)
{
#if 0
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
#endif
	return 0;
}

int fuse_reply_write(fuse_req_t req, size_t count)
{
	struct fuse_write_out arg;

	memset(&arg, 0, sizeof(arg));
	arg.size = count;

	return send_reply_ok(req, &arg, sizeof(arg));
}
