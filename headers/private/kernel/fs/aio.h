/*
 * Copyright 2022 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#ifndef _KERNEL_AIO_H
#define _KERNEL_AIO_H

#include <aio.h>


#ifdef __cplusplus
extern "C" {
#endif

extern int _user_aio_read(struct aiocb *iocb);
extern int _user_aio_write(struct aiocb *iocb);
extern int _user_lio_listio(int mode, struct aiocb * const *list, int nent, struct sigevent *sig);
extern int _user_aio_error(const struct aiocb *iocb);
extern ssize_t _user_aio_return(struct aiocb *iocb);
extern int _user_aio_cancel(int fildes, struct aiocb *iocb);
extern int _user_aio_suspend(const struct aiocb *const *iocbs, int niocb, bigtime_t timeout);
extern int _user_aio_fsync(int op, struct aiocb *iocb);

#ifdef __cplusplus
}
#endif

#endif	// _KERNEL_AIO_H
