/*
 * Copyright 2019, Rob Gill, rrobgill@protonmail.com
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <FindDirectory.h>
#include <StorageDefs.h>

#include <errno_private.h>
#include <find_directory_private.h>


static status_t
get_path(char *path, bool create)
{
	status_t status = __find_directory(B_SYSTEM_SETTINGS_DIRECTORY, -1, create,
		path, B_PATH_NAME_LENGTH);
	if (status != B_OK)
		return status;

	if (create)
		mkdir(path, 0755);
	strlcat(path, "/hostid", B_PATH_NAME_LENGTH);
	return B_OK;
}


extern "C" int
gethostid(void)
{
	// look up hostid from settings hostid file, containing
	// hostid formatted as 32 bit long hex, zero padded
	// eg 0x0123abcd

	char path[B_PATH_NAME_LENGTH];
	if (get_path(path, false) != B_OK) {
		__set_errno(B_ERROR);
		return -1;
	}

	int idStringSize = 11;
	char hostIdString[11];
	char * end;

	int file = open(path, O_RDONLY);
	if (file < 0) {
		__set_errno(B_ERROR);
		return -1;
	}

	int length = read(file, hostIdString, idStringSize);
	close(file);

	if (length < 0) {
		__set_errno(B_ERROR);
		return -1;
	}

	long hostId = strtol(hostIdString, &end, 0);
	return (hostId);
}

