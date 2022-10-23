/*
 * Copyright 2022 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _AIO_H
#define _AIO_H


#include <signal.h>


// aio_cancel return values
#define	AIO_CANCELED	1
#define	AIO_NOTCANCELED	2
#define	AIO_ALLDONE		3

// aiocb.aio_lio_opcode
#define	LIO_NOP		0
#define LIO_WRITE	1
#define	LIO_READ	2

// lio_listio.mode
#define	LIO_NOWAIT	0
#define	LIO_WAIT	1


typedef struct aiocb {
	int				aio_fildes;
	off_t			aio_offset;
	volatile void*	aio_buf;
	size_t			aio_nbytes;
	int				aio_reqprio;
	int				aio_lio_opcode;
	int				aio_object_type; // B_OBJECT_TYPE_*
	int				aio_status;
	int				aio_error;
	struct sigevent	aio_sigevent;
} aiocb_t;


#ifdef __cplusplus
extern "C" {
#endif

int aio_read(struct aiocb *iocb);
int aio_write(struct aiocb *iocb);
int lio_listio(int mode, struct aiocb * const *list, int nent, struct sigevent *sig);
int aio_error(const struct aiocb *iocb);
ssize_t aio_return(struct aiocb *iocb);
int aio_cancel(int fildes, struct aiocb *iocb);
int aio_suspend(const struct aiocb *const *iocbs, int niocb, const struct timespec *timeout);
int aio_fsync(int op, struct aiocb *iocb);

#ifdef __cplusplus
}
#endif

#endif /* _AIO_H */
