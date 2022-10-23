/*
 * Copyright 2022 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>
#include <fs/aio.h>


int
_user_aio_read(struct aiocb *iocb)
{
	return ENOSYS;
}


int
_user_aio_write(struct aiocb *iocb)
{
	return ENOSYS;
}


int
_user_lio_listio(int mode, struct aiocb * const *list, int nent, struct sigevent *sig)
{
	return ENOSYS;
}


int
_user_aio_error(const struct aiocb *iocb)
{
	return ENOSYS;
}


ssize_t
_user_aio_return(struct aiocb *iocb)
{
	return ENOSYS;
}


int
_user_aio_cancel(int fildes, struct aiocb *iocb)
{
	return ENOSYS;
}


int
_user_aio_suspend(const struct aiocb *const *iocbs, int niocb, bigtime_t timeout)
{
	return ENOSYS;
}


int
_user_aio_fsync(int op, struct aiocb *iocb)
{
	return ENOSYS;
}
