/*
 * Copyright 2002-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYS_UIO_H
#define _SYS_UIO_H


#include <sys/types.h>


typedef struct iovec {
	void  *iov_base;
	size_t iov_len;
} iovec;


#ifdef __cplusplus
extern "C" {
#endif

ssize_t readv(int fd, const struct iovec *vector, int count);
ssize_t readv_pos(int fd, off_t pos, const struct iovec *vec, int count);
ssize_t writev(int fd, const struct iovec *vector, int count);
ssize_t writev_pos(int fd, off_t pos, const struct iovec *vec, int count);

#ifdef _DEFAULT_SOURCE
ssize_t preadv(int fd, const struct iovec * vector, int count, off_t pos);
ssize_t pwritev(int fd, const struct iovec * vector, int count, off_t pos);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _SYS_UIO_H */
