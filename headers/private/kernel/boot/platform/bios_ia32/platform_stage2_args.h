/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef KERNEL_BOOT_PLATFORM_BIOS_IA32_STAGE2_H
#define KERNEL_BOOT_PLATFORM_BIOS_IA32_STAGE2_H

#ifndef KERNEL_BOOT_STAGE2_ARGS_H
#	error This file is included from <boot/stage2_args.h> only
#endif

#include <boot/disk_identifier.h>

typedef struct {
	uint8			drive_id;
	check_sum		checksum[NUM_DISK_CHECK_SUMS];
} _PACKED bios_drive_checksum;

struct platform_stage2_args {
	bios_drive_checksum	*bios_drives_checksums;
};

#endif	/* KERNEL_BOOT_PLATFORM_BIOS_IA32_STAGE2_H */
