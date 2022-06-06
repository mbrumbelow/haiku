/*
 * Copyright 2022, Adrien Destugues <pulkomandy@pulkomandy.tk>
 * Distributed under terms of the MIT license.
 */

#include "FUSELowLevel.h"

#include <assert.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>

#include "fuse_api.h"
#include "fuse_kernel.h"


static const struct fuse_lowlevel_ops* sLowLevelOps;

// Storage for replies
struct fuse_req {
	fuse_req()
		: fReplyResult(0),
		fReplyBuf(NULL)
	{
		sem_init(&fSyncSem, 0, 0);
	}

	~fuse_req() {
		sem_destroy(&fSyncSem);
	}

	void Wait() {
		sem_wait(&fSyncSem);
	}

	void Notify() {
		sem_post(&fSyncSem);
	}

	sem_t fSyncSem;
	ssize_t fReplyResult;

	union {
		struct stat* fReplyAttr;
		struct fuse_entry_param fReplyEntry;
		struct fuse_file_info* fReplyOpen;
		struct statvfs* fReplyStat;

		char* fReplyBuf;
	};
};


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
		fuse_req request;
		sLowLevelOps->lookup(&request, ino, pathPointer);
		request.Wait();
		if (request.fReplyResult != 0) {
			fprintf(stderr, "Failed to lookup %s: %s\n", path, strerror(request.fReplyResult));
			free(pathCopy);
			return request.fReplyResult;
		}
		ino = request.fReplyEntry.ino;
		pathPointer = strtok(NULL, "/");
	}
	free(pathCopy);
	return ino;
}


static int
GetAttr(const char* path, struct stat* reply)
{
	fuse_ino_t ino = TraversePath(FUSE_ROOT_ID, path);

	fuse_req request;
	request.fReplyAttr = reply;
	sLowLevelOps->getattr(&request, ino, NULL);
	request.Wait();
	return request.fReplyResult;
}


static int
Open(const char* path, fuse_file_info* ffi)
{
	fuse_ino_t ino = TraversePath(FUSE_ROOT_ID, path);

	fuse_req request;
	request.fReplyOpen = ffi;
	sLowLevelOps->open(&request, ino, ffi);
	request.Wait();
	return request.fReplyResult;
}


static int
Read(const char* path, char* buffer, size_t bufferSize, off_t position, fuse_file_info* ffi)
{
	fuse_ino_t ino = TraversePath(FUSE_ROOT_ID, path);

	fuse_req request;
	request.fReplyBuf = buffer;
	sLowLevelOps->read(&request, ino, bufferSize, position, ffi);
	request.Wait();
	return request.fReplyResult;
}


static int
Statfs(const char* path, struct statvfs* stat)
{
	fuse_ino_t ino = TraversePath(FUSE_ROOT_ID, path);

	fuse_req request;
	request.fReplyStat = stat;
	sLowLevelOps->statfs(&request, ino);
	request.Wait();
	return request.fReplyResult;
}


static int
Release(const char* path, fuse_file_info* ffi)
{
	fuse_ino_t ino = TraversePath(FUSE_ROOT_ID, path);

	fuse_req request;
	sLowLevelOps->release(&request, ino, ffi);
	request.Wait();
	return request.fReplyResult;
}


static int
ReadDir(const char* path, void* cookie, fuse_fill_dir_t filler, off_t pos, fuse_file_info* ffi)
{
	fuse_ino_t ino = TraversePath(FUSE_ROOT_ID, path);

	fuse_req request;
	char buffer[PAGESIZE];
	request.fReplyBuf = buffer;

	const char* bufferPointer = buffer;
	struct fuse_dirent* dirent = (struct fuse_dirent*)bufferPointer;
	dirent->off = pos;
	for(;;) {
		memset(buffer, 0, PAGESIZE);
		sLowLevelOps->readdir(&request, ino, PAGESIZE, pos, ffi);
		request.Wait();
		// Have we reached end of directory yet?
		if (request.fReplyResult <= 0)
			return request.fReplyResult;
		bufferPointer = request.fReplyBuf;
		do {
			dirent = (struct fuse_dirent*)bufferPointer;
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
		} while (bufferPointer < (request.fReplyBuf + request.fReplyResult));
	}
	return 0;
}


static void*
Init(struct fuse_conn_info* conn)
{
	fuse_req request;
	sLowLevelOps->init(&request, conn);
	// No reply for this one
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
	*req->fReplyAttr = *attr;
	req->Notify();
	return 0;
}


int
fuse_reply_buf(fuse_req_t req, const char *buf, size_t size)
{
	memcpy(req->fReplyBuf, buf, size);
	req->fReplyResult = size;
	req->Notify();
	return 0;
}


int
fuse_reply_entry(fuse_req_t req, const struct fuse_entry_param *e)
{
	req->fReplyEntry = *e;
	req->Notify();
	return 0;
}


int
fuse_reply_err(fuse_req_t req, int err)
{
	assert(err >= 0);
	req->fReplyResult = -err;
	req->Notify();
	return 0;
}


int
fuse_reply_open(fuse_req_t req, const struct fuse_file_info* f)
{
	*req->fReplyOpen = *f;
	req->Notify();
	return 0;
}


int
fuse_reply_statfs(fuse_req_t req, const struct statvfs* stat)
{
	*req->fReplyStat = *stat;
	req->Notify();
	return 0;
}


struct fuse_operations*
FUSELowLevelWrapper(const struct fuse_lowlevel_ops* lowLevelOps, size_t opsSize)
{
	sLowLevelOps = lowLevelOps;
	return &sHighLevelOps;
}
