/*
 * Copyright 2021, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Jérôme Duval
 */


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <OS.h>

#include "kcov.h"


#define COVER_SIZE (16 << 20)


int
main(int argc, char** argv)
{
	int fd = open("/dev/misc/kcov", O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "couldn't open kcov\n");
		exit(1);
	}
	int count = COVER_SIZE;
	if (ioctl(fd, KIOSETBUFSIZE, &count, sizeof(count)) != 0) {
		fprintf(stderr, "couldn't set buffer size (0x%x)\n", errno);
		exit(1);
	}

	area_id area;
	if (ioctl(fd, KIOGETAREA, &area, sizeof(area)) != 0) {
		fprintf(stderr, "couldn't get area (0x%x)\n", errno);
		exit(1);
	}

	uint64* cover;
	area_id cloned = clone_area("kcov clone", (void **)&cover, B_ANY_ADDRESS,
		B_READ_AREA | B_WRITE_AREA, area);
	if (cloned < B_OK) {
		fprintf(stderr, "couldn't clone area (0x%x)\n", cloned);
		exit(1);
	}

	int mode = KCOV_MODE_TRACE_PC;
	if (ioctl(fd, KIOENABLE, &mode, sizeof(mode)) != 0) {
		fprintf(stderr, "couldn't enable coverage\n");
		exit(1);
	}
	cover[0] = 0;

	read(-1, NULL, 0);

	int64 total = cover[0];
	if (ioctl(fd, KIODISABLE, NULL, 0) != 0) {
		fprintf(stderr, "couldn't disable coverage\n");
		exit(1);
	}
	for (int i = 0; i < total; i++)
		printf("%p\n", (void*)cover[i + 1]);
	delete_area(cloned);
	close(fd);
	return 0;
}
