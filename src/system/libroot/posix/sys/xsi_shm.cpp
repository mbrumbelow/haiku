/*
 * Copyright 2011, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		David Höppner <0xffea@gmail.com>
 */


#include <sys/shm.h>

#include <errno.h>

#include <syscall_utils.h>
#include <syscalls.h>


void*
shmat(int key, const void* address, int flags)
{
	void* returnAddress;

	status_t status = _kern_xsi_shmat(key, address, flags, &returnAddress);
	if (status != B_OK) {
		errno = status;
		return ((void*)-1);
	}

	return returnAddress;
}


int
shmctl(int id, int command, struct shmid_ds* buffer)
{
	RETURN_AND_SET_ERRNO(_kern_xsi_shmctl(id, command, buffer));
}


int
shmdt(const void* address)
{
	RETURN_AND_SET_ERRNO(_kern_xsi_shmdt(address));
}


int
shmget(key_t key, size_t size, int flags)
{
	RETURN_AND_SET_ERRNO(_kern_xsi_shmget(key, size, flags));
}
