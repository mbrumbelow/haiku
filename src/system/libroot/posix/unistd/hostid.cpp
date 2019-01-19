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


extern "C" int
gethostid(void)
{
	// look up hostid from settings hostid file
	char path[B_PATH_NAME_LENGTH];
	status_t status = __find_directory(B_SYSTEM_SETTINGS_DIRECTORY, -1, false,
		path, B_PATH_NAME_LENGTH);
	if (status != B_OK)
		return 0;

	strlcat(path, "/hostid", B_PATH_NAME_LENGTH);

	int file = open(path, O_RDONLY);
	if (file < 0)
		return 0;

	char hostIdString[12];
	int length = read(file, hostIdString, sizeof(hostIdString) - 1);
	close(file);

	if (length <= 0)
		return 0;

	// Make sure it is NULL-terminated
	hostIdString[length] = '\0';
	char* end;
	long hostId = strtol(hostIdString, &end, 0);
	// Check if we could parse something
	if (end == hostIdString)
		return 0;

	return hostId;
}

