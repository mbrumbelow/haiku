/*
 * Copyright 2008-2011, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Salvatore Benedetto <salvatore.benedetto@gmail.com>
 *		David Höppner <0xffea@gmail.com>
 */


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>

#include <OS.h>

#include "TestUnitUtils.h"


#define KEY		((key_t)12345)
#define SHM_SIZE	16*1024*1024


static void
test_shmget()
{
	TEST_SET("shmget({IPC_PRIVATE, key})");

	const char* currentTest = NULL;

	// Open private set with IPC_PRIVATE
	TEST("shmget(IPC_PRIVATE) - private");
	int shmID = shmget(IPC_PRIVATE, SHM_SIZE, S_IRUSR | S_IWUSR);
	assert_posix_bool_success(shmID != -1);

	// Destroy private memory entry
	TEST("shmctl(IPC_RMID) - private");
	status_t status = shmctl(shmID, IPC_RMID, NULL);
	assert_posix_bool_success(status != -1);

	// Open non-private non-existing set with IPC_CREAT
	TEST("shmget(KEY, IPC_CREAT) non-existing");
	shmID = shmget(KEY, SHM_SIZE, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR
		| S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	assert_posix_bool_success(status != -1);

	// Re-open non-private existing with greater size 
	TEST("shmget(KEY) re-open existing without IPC_CREAT");
	int returnID = shmget(KEY, SHM_SIZE+1, 0);
	assert_posix_bool_success(errno == EINVAL);

	// Re-open non-private existing without IPC_CREAT
	TEST("shmget(KEY) re-open existing without IPC_CREAT");
	returnID = shmget(KEY, SHM_SIZE, 0);
	assert_equals(shmID, returnID);

	// Re-open non-private existing with IPC_CREAT
	TEST("shmget(IPC_CREATE) re-open existing with IPC_CREAT");
	returnID = shmget(KEY, SHM_SIZE, IPC_CREAT | IPC_EXCL);
	assert_posix_bool_success(errno == EEXIST);

	// Destroy non-private
	TEST("shmctl(IPC_RMID)");
	status = shmctl(shmID, IPC_RMID, NULL);
	assert_posix_bool_success(status != -1);

	// Open non-private non-existing without IPC_CREAT
	TEST("shmget(IPC_CREATE) non-existing without IPC_CREAT");
	shmID = shmget(KEY, SHM_SIZE, IPC_EXCL | S_IRUSR | S_IWUSR
		| S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	assert_posix_bool_success(errno == ENOENT);

	// Destroy non-existing
	TEST("shmctl()");
	status = shmctl(shmID, IPC_RMID, NULL);
	assert_posix_bool_success(errno == EINVAL);

	TEST("done");
}


static void
test_shmctl()
{
        TEST_SET("shmctl({IPC_STAT, IPC_SET, IPC_RMID})");

        const char* currentTest = NULL;

	// Read shmid data structure for existing id
        TEST("shmctl(ID, IPC_STAT) read shmid data structure for existing id");
	int shmID = shmget(IPC_PRIVATE, SHM_SIZE, S_IRUSR | S_IWUSR);
	assert_posix_bool_success(shmID != -1);
	struct shmid_ds mds;
	status_t status = shmctl(shmID, IPC_STAT, &mds);
	assert_posix_bool_success(shmID != -1);
	assert_equals(mds.shm_cpid, getpid());

	// Cache has still areas
	TEST("shmctl(ID, IPC_RMID) remove attached id");
	shmat(shmID, NULL, 0);
	shmctl(shmID, IPC_RMID, 0);

	TEST("done");
}


static void
test_shmdt()
{
	TEST_SET("shmdt(NULL, ID)");

	const char* currentTest = NULL;

	TEST("shmctl(NULL) detach NULL pointer");
	status_t status = shmdt(NULL);
	assert_posix_bool_success(errno == EINVAL);

	TEST("shmctl(ID) detach existing ID");
	int shmID = shmget(IPC_PRIVATE, SHM_SIZE, S_IRUSR);
	void* address = shmat(shmID, NULL, 0);
	status = shmdt(address);
	assert_posix_bool_success(status != -1);

	TEST("done");
}


int
main()
{
	test_shmget();
	test_shmdt();
	test_shmctl();

	printf("\nAll tests OK\n");
}
