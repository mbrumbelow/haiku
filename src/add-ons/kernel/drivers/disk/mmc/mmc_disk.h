/*
 * Copyright 2018 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		B Krishnan Iyer, krishnaniyer97@gmail.com
 */
#ifndef _MMC_DISK_H
#define _MMC_DISK_H


#include <device_manager.h>
#include <KernelExport.h>


typedef struct {
	device_node* 	node;
	mmc_driver_interface* mmc;
	void* mmc_device;
} mmc_disk_driver_info;


typedef struct {

} mmc_disk_handle;


#endif /*_MMC_DISK_H*/
