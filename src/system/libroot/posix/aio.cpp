/*
 * Copyright 2022 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <aio.h>

#include <errno_private.h>
#include <syscalls.h>
#include <syscall_utils.h>


int
aio_read(struct aiocb *iocb)
{
	RETURN_AND_SET_ERRNO(_kern_aio_read(iocb));
}


int
aio_write(struct aiocb *iocb)
{
	RETURN_AND_SET_ERRNO(_kern_aio_write(iocb));
}


int
lio_listio(int mode, struct aiocb * const list[], int nent, struct sigevent *sig)
{
	RETURN_AND_SET_ERRNO(_kern_lio_listio(mode, list, nent, sig));
}


int
aio_error(const struct aiocb *iocb)
{
	RETURN_AND_SET_ERRNO(_kern_aio_error(iocb));
}


ssize_t
aio_return(struct aiocb *iocb)
{
	RETURN_AND_SET_ERRNO(_kern_aio_return(iocb));
}


int
aio_cancel(int fildes, struct aiocb *iocb)
{
	RETURN_AND_SET_ERRNO(_kern_aio_cancel(fildes, iocb));
}


int
aio_suspend(const struct aiocb *const iocbs[], int niocb, const struct timespec *timeout)
{
	bigtime_t timeoutMicros;
	if (timeout == 0)
		timeoutMicros = B_INFINITE_TIMEOUT;
	else {
		if (timeout->tv_nsec < 0 || timeout->tv_nsec >= 1000 * 1000 * 1000)
			RETURN_AND_SET_ERRNO(EINVAL);

		timeoutMicros = ((bigtime_t)timeout->tv_sec) * 1000000 + timeout->tv_nsec / 1000;
	}

	RETURN_AND_SET_ERRNO(_kern_aio_suspend(iocbs, niocb, timeoutMicros));
}


int
aio_fsync(int op, struct aiocb *iocb)
{
	RETURN_AND_SET_ERRNO(_kern_aio_fsync(op, iocb));
}
