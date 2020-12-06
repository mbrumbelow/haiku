/*
 * Copyright 2018-2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		B Krishnan Iyer, krishnaniyer97@gmail.com
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */
#ifndef _MMC_DISK_H
#define _MMC_DISK_H


#include <device_manager.h>
#include <KernelExport.h>

#include <stdint.h>

#include <mmc.h>

#include "IOSchedulerSimple.h"


// This is the device info structure, allocated once per device
typedef struct {
	device_node* node;
	device_node* parent;
	mmc_device_interface* mmc;
	uint16_t rca;

	size_t block_size;
	uint64_t capacity;

	DMAResource* dmaResource;
	IOScheduler* scheduler;
} mmc_disk_driver_info;


// This is allocated once per open() call on the device (there can be multiple
// open file descriptors for the same device)
typedef struct {
	mmc_disk_driver_info* info;
} mmc_disk_handle;


#endif /*_MMC_DISK_H*/
