/*
 * Copyright 2022, Adrien Destugues <pulkomandy@pulkomandy.tk>
 * Distributed under terms of the MIT license.
 */

#include "FUSELowLevel.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "fuse_api.h"
#include "fuse_kernel.h"


static const struct fuse_lowlevel_ops* sLowLevelOps;

// Storage for replies
// FIXME by having this static, we can handle only one reply at a time.
// This should be stored in the fuse_req structure passed to each request and reply instead, and
// we could then allocate a separate instance for each request (possibly allocated on the stack,
// if the replies re always synchronous)
static int sReplyErr;
static struct stat sReplyAttr = {0};
static struct fuse_entry_param sReplyEntry;
static struct fuse_file_info sReplyOpen;
static struct statvfs* sReplyStat;

static char* sReplyBuf;
static size_t sReplySize;


//#pragma mark - Implementation of highlevel ops in terms of lowlevel ones


// FIXME this is needed only because we go through the "highlevel" FUSE API. We could wire more
// directly into userlandfs to get a hand on its inodes, which would work better, provided that
// -o use_ino is used and the filesystem under use has unique inodes we can actually use for that.
static fuse_ino_t
TraversePath(fuse_ino_t base, const char* path)
{
	char* pathCopy = strdup(path);
	char* pathPointer = strtok(pathCopy, "/");
	fuse_ino_t ino = base;
	while (pathPointer != NULL) {
		sReplyErr = 0;
		sLowLevelOps->lookup(0, ino, pathPointer);
		if (sReplyErr != 0) {
			fprintf(stderr, "Failed to lookup %s: %s\n", path, strerror(sReplyErr));
			free(pathCopy);
			return sReplyErr;
		}
		ino = sReplyEntry.ino;
		pathPointer = strtok(NULL, "/");
	}
	free(pathCopy);
	return ino;
}


static int
GetAttr(const char* path, struct stat* reply)
{
	fuse_ino_t ino = TraversePath(FUSE_ROOT_ID, path);

	sReplyErr = 0;
	sLowLevelOps->getattr(0, ino, NULL);
	if (sReplyErr != 0)
		return sReplyErr;
	*reply = sReplyAttr;
	return 0;
}


static int
Open(const char* path, fuse_file_info* ffi)
{
	fuse_ino_t ino = TraversePath(FUSE_ROOT_ID, path);

	sReplyErr = 0;
	sLowLevelOps->open(0, ino, ffi);
	if (sReplyErr != 0)
		return sReplyErr;

	*ffi = sReplyOpen;
	return 0;
}


static int
Read(const char* path, char* buffer, size_t bufferSize, off_t position, fuse_file_info* ffi)
{
	fuse_ino_t ino = TraversePath(FUSE_ROOT_ID, path);

	sReplyErr = 0;
	sReplyBuf = buffer;
	sLowLevelOps->read(0, ino, bufferSize, position, ffi);
	if (sReplyErr != 0)
		return sReplyErr;

	return sReplySize;
}


static int
Statfs(const char* path, struct statvfs* stat)
{
	fuse_ino_t ino = TraversePath(FUSE_ROOT_ID, path);

	sReplyErr = 0;
	sReplyStat = stat;
	sLowLevelOps->statfs(0, ino);
	if (sReplyErr != 0)
		return sReplyErr;

	return 0;
}


static int
Release(const char* path, fuse_file_info* ffi)
{
	fuse_ino_t ino = TraversePath(FUSE_ROOT_ID, path);

	sReplyErr = 0;
	sLowLevelOps->release(0, ino, ffi);
	if (sReplyErr != 0)
		return sReplyErr;

	return 0;
}


static int
ReadDir(const char* path, void* cookie, fuse_fill_dir_t filler, off_t pos, fuse_file_info* ffi)
{
	fuse_ino_t ino = TraversePath(FUSE_ROOT_ID, path);

	sReplyErr = 0;
	char buffer[PAGESIZE];
	sReplyBuf = buffer;

	const char* bufferPointer = sReplyBuf;
	struct fuse_dirent* dirent = (struct fuse_dirent*)bufferPointer;
	dirent->off = pos;
	for(;;) {
		memset(sReplyBuf, 0, PAGESIZE);
		fprintf(stderr, "READDIR OFF %lx\n", pos);
		sLowLevelOps->readdir(0, ino, PAGESIZE, pos, ffi);
		// Have we reached end of directory yet?
		if (sReplySize == 0)
			return 0;
		bufferPointer = sReplyBuf;
		do {
			dirent = (struct fuse_dirent*)bufferPointer;
			fprintf(stderr, "%p [%lx/%lx]: ino %lx type %x off %lx '%s' %d %ld\n", dirent,
				bufferPointer - sReplyBuf, sReplySize,
				dirent->ino, dirent->type, dirent->off, dirent->name, dirent->namelen, strlen(dirent->name));
			struct stat info;
			info.st_ino = dirent->ino;
			info.st_mode = dirent->type << 12;

			// make sure we have a null-terminated name (the one in dirent isn't...)
			char namebuffer[dirent->namelen + 1];
			strlcpy(namebuffer, dirent->name, dirent->namelen + 1);
			int ret = filler(cookie, namebuffer, &info, pos);
			if (ret != 0)
				return ret;

			pos = dirent->off;
			bufferPointer += FUSE_DIRENT_SIZE(dirent);
		} while (bufferPointer < (sReplyBuf + sReplySize));
	}
	return 0;
}


static void*
Init(struct fuse_conn_info* conn)
{
	sLowLevelOps->init(NULL, conn);
	return NULL;
}


static struct fuse_operations sHighLevelOps = {
	GetAttr,
	NULL, // readlink
	NULL, // getdir, not needed, readdir is used instead
	NULL, // TODO mknod
	NULL, // TODO mkdir
	NULL, // TODO unlink
	NULL, // TODO rmdir
	NULL, // symlink
	NULL, // TODO rename
	NULL, // link
	NULL, // chmod
	NULL, // chown
	NULL, // truncate
	NULL, // utime
	Open,
	Read,
	NULL, // TODO write
	Statfs,
	NULL, // flush
	Release,
	NULL, // fsync
	NULL, // setxattr
	NULL, // getxattr
	NULL, // listxattr,
	NULL, // removexattr
	NULL, // opendir
	ReadDir,
	NULL, // releasedir
	NULL, // fsyncdir
	Init,
	NULL, // destroy
	NULL, // access
	NULL, // TODO create
	NULL, // ftruncate
	NULL, // fgetattr
	NULL, // lock
	NULL, // utimens
	NULL, // bmap
	NULL, // FIXME remove get_fs_info
	false, // flag_nullpath_ok
	false, // flag_nopath
	false, // flag_utile_iomit_ok
	0, // reserved
	NULL, // ioctl
	NULL, // poll
	NULL, // write_buf
	NULL, // read_buf
	NULL, // flock
	NULL, // fallocate
};


//#pragma mark - lowlevel replies handling


int
fuse_reply_attr(fuse_req_t req, const struct stat *attr, double attr_timeout)
{
	fprintf(stderr, "Got an ATTR reply\n");
	sReplyAttr = *attr;
	return 0;
}


int
fuse_reply_buf(fuse_req_t req, const char *buf, size_t size)
{
	fprintf(stderr, "Got a BUF reply\n");
	assert(size <= PAGESIZE);
	memcpy(sReplyBuf, buf, size);
	sReplySize = size;
	return 0;
}


int
fuse_reply_entry(fuse_req_t req, const struct fuse_entry_param *e)
{
	fprintf(stderr, "Got an ENTRY reply\n");
	sReplyEntry = *e;
	return 0;
}


int
fuse_reply_err(fuse_req_t req, int err)
{
	fprintf(stderr, "Got an ERR reply\n");
	sReplyErr = -err;
	return 0;
}


int
fuse_reply_open(fuse_req_t req, const struct fuse_file_info* f)
{
	sReplyOpen = *f;
	return 0;
}


int
fuse_reply_statfs(fuse_req_t req, const struct statvfs* stat)
{
	*sReplyStat = *stat;
	return 0;
}


struct fuse_operations*
FUSELowLevelWrapper(const struct fuse_lowlevel_ops* lowLevelOps, size_t opsSize)
{
	assert(sLowLevelOps == NULL);
	sLowLevelOps = lowLevelOps;
	return &sHighLevelOps;
}
