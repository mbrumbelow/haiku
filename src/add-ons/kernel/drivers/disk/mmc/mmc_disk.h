/*
 * Copyright 2018-2019 Haiku, Inc. All rights reserved.
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

typedef struct {
	driver_module_info stdops;

	// TODO specific ops provided by MMC bus go here (for sending commands, etc)
} mmc_driver_interface;

typedef struct {
	device_node* 	node;
	mmc_driver_interface* mmc;
	void* mmc_device;

	size_t block_size;
	uint32_t capacity;
} mmc_disk_driver_info;


typedef struct {
	mmc_disk_driver_info* info;
} mmc_disk_handle;


#endif /*_MMC_DISK_H*/
