/*
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 * Copyright 2000 VA Linux Systems, Inc., Sunnyvale, California.
 * (drm_ioctl) All Rights Reserved.
 *
 * Copyright 2020-2021, Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT license
 *
 * Authors:
 *    Keith Packard <keithp@keithp.com>
 *    Alexander von Gluck IV <kallisti5@unixzen.com>
 */


#include <drm.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>


#define ERROR(x...) printf("Error: " x)


static int
drm_ioctl(int handle, int command, void* data)
{
	int result = -1;
	do {
		result = ioctl(handle, command, data);
	} while (result == -1 && (errno == EINTR || errno == EAGAIN));
	return result;
}


static bool
probe_device_version(int fd)
{
	struct drm_version version;
	if (drm_ioctl(fd, DRM_IOCTL_VERSION, &version)) {
		ERROR("%s: unable to locate DRM version API on device\n", __func__);
		return false;
	}

	printf("    - DRM Version     '%d.%d.%d'\n",
		version.version_major, version.version_minor,
		version.version_patchlevel);

	if (version.name_len)
		version.name = (char*)malloc(version.name_len);
	if (version.desc_len)
		version.desc = (char*)malloc(version.desc_len);
	if (version.date_len)
		version.date = (char*)malloc(version.date_len);

	if (drm_ioctl(fd, DRM_IOCTL_VERSION, &version)) {
		ERROR("%s: unable to locate DRM version API on device\n", __func__);
		return false;
	}

	printf("    - DRM Name        '%s'\n", version.name);
	printf("    - DRM Description '%s'\n", version.desc);
	printf("    - DRM Date        '%s'\n", version.date);

	return true;
}


int
main(int argc, char* argv[])
{
	int fd = -1;

	if (argc != 2) {
		printf("Usage: drminfo <device>\n");
		return 1;
	}

	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		printf("Error: Unable to access %s\n", argv[1]);
		return 1;
	}

	printf("  + Graphics Device: %s\n", argv[1]);
	probe_device_version(fd);
	close(fd);

	return 0;
}
