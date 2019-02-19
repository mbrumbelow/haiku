/*
 * Copyright 2018-2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		B Krishnan Iyer, krishnaniyer97@gmail.com
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */

#include <new>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "mmc_disk.h"
#include "mmc_icon.h"
#include "mmc.h"

#include <drivers/device_manager.h>
#include <drivers/KernelExport.h>
#include <drivers/Drivers.h>
#include <kernel/OS.h>

// #include <fs/devfs.h>

#define TRACE_MMC_DISK
#ifdef TRACE_MMC_DISK
#	define TRACE(x...) dprintf("\33[33mmmc_disk:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#define ERROR(x...)			dprintf("\33[33mmmc_disk:\33[0m " x)
#define CALLED() 			TRACE("CALLED %s\n", __PRETTY_FUNCTION__)

#define MMC_DISK_DRIVER_MODULE_NAME "drivers/disk/mmc/mmc_disk/driver_v1"
#define MMC_DISK_DEVICE_MODULE_NAME "drivers/disk/mmc/mmc_disk/device_v1"
#define MMC_DEVICE_ID_GENERATOR "mmc/device_id"

static device_manager_info* sDeviceManager;


static float
mmc_disk_supports_device(device_node* parent)
{
	// Filter all devices that are not on an MMC bus
	const char* bus;
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus,
			true) != B_OK)
		return -1;

	if (strcmp(bus, "mmc") != 0)
		return 0.0;

	CALLED();

	// Filter all devices that are not of the known types
	uint8_t deviceType;
	if (sDeviceManager->get_attr_uint8(parent, "mmc/type",
			&deviceType, true) != B_OK)
	{
		ERROR("Could not get device type\n");
		return -1;
	}

	if (deviceType != CARD_TYPE_SD && deviceType != CARD_TYPE_SDHC)
		return 0.0;

	TRACE("sdhci device found, parent: %p\n", parent);

	return 0.8;
}


static status_t
mmc_disk_register_device(device_node* node)
{
	CALLED();

	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { string: "SD Card" }},
		{ NULL }
	};

	return sDeviceManager->register_node(node
		, MMC_DISK_DRIVER_MODULE_NAME, attrs, NULL, NULL);
}


static status_t
mmc_disk_init_driver(device_node* node, void** cookie)
{
	CALLED();
	mmc_disk_driver_info* info = (mmc_disk_driver_info*)malloc(
		sizeof(mmc_disk_driver_info));

	if (info == NULL)
		return B_NO_MEMORY;

	memset(info, 0, sizeof(*info));

	info->node = node;

	*cookie = info;
	return B_OK;
}


static void
mmc_disk_uninit_driver(void* _cookie)
{
	CALLED();
	mmc_disk_driver_info* info = (mmc_disk_driver_info*)_cookie;
	free(info);
}


static status_t
mmc_disk_register_child_devices(void* _cookie)
{
	CALLED();
	mmc_disk_driver_info* info = (mmc_disk_driver_info*)_cookie;
	status_t status;

	int32 id = sDeviceManager->create_id(MMC_DEVICE_ID_GENERATOR);
	if (id < 0)
		return id;

	char name[64];
	snprintf(name, sizeof(name), "disk/mmc/%" B_PRId32 "/raw", id);

	status = sDeviceManager->publish_device(info->node, name,
		MMC_DISK_DEVICE_MODULE_NAME);

	return status;
}


//	#pragma mark - device module API


static status_t
mmc_block_init_device(void* _info, void** _cookie)
{
	CALLED();
	mmc_disk_driver_info* info = (mmc_disk_driver_info*)_info;

	device_node* parent = sDeviceManager->get_parent_node(info->node);
	sDeviceManager->get_driver(parent, (driver_module_info **)&info->mmc,
		(void **)&info->mmc_device);
	sDeviceManager->put_node(parent);

	status_t status = B_OK;
	// TODO Get capacity

	return status;
}


static void
mmc_block_uninit_device(void* _cookie)
{
	CALLED();
	//mmc_disk_driver_info* info = (mmc_disk_driver_info*)_cookie;

	// TODO cleanup whatever is relevant
}


static status_t
mmc_block_open(void* _info, const char* path, int openMode, void** _cookie)
{
	CALLED();
	mmc_disk_driver_info* info = (mmc_disk_driver_info*)_info;

	// TODO allocate cookie
	mmc_disk_handle* handle = new(std::nothrow) mmc_disk_handle;
	*_cookie = handle;
	if (handle == NULL) {
		return B_NO_MEMORY;
	}
	handle->info = info;

	return B_OK;
}


static status_t
mmc_block_close(void* cookie)
{
	mmc_disk_handle* handle = (mmc_disk_handle*)cookie;
	CALLED();

	return B_OK;
}


static status_t
mmc_block_free(void* cookie)
{
	CALLED();
	mmc_disk_handle* handle = (mmc_disk_handle*)cookie;

	delete handle;
	return B_OK;
}


static status_t
mmc_block_ioctl(void* cookie, uint32 op, void* buffer, size_t length)
{
	CALLED();
	mmc_disk_handle* handle = (mmc_disk_handle*)cookie;
	mmc_disk_driver_info* info = handle->info;

	TRACE("ioctl(op = %ld)\n", op);

	switch (op) {
		case B_GET_MEDIA_STATUS:
		{
			if (buffer == NULL || length < sizeof(status_t))
				return B_BAD_VALUE;

			*(status_t *)buffer = B_OK;
			TRACE("B_GET_MEDIA_STATUS: 0x%08lx\n", *(status_t *)buffer);
			return B_OK;
			break;
		}

		case B_GET_DEVICE_SIZE:
		{
			size_t size = info->capacity * info->block_size;
			return user_memcpy(buffer, &size, sizeof(size_t));
		}

		case B_GET_GEOMETRY:
		{
			if (buffer == NULL || length < sizeof(device_geometry))
				return B_BAD_VALUE;

		 	device_geometry geometry;
			// TODO status_t status = get_geometry(handle, &geometry);
			status_t status = B_ERROR;
			if (status != B_OK)
				return status;

			return user_memcpy(buffer, &geometry, sizeof(device_geometry));
		}

		case B_GET_ICON_NAME:
			return user_strlcpy((char*)buffer, "devices/drive-harddisk",
				B_FILE_NAME_LENGTH);

		case B_GET_VECTOR_ICON:
		{
			// TODO: take device type into account!
			device_icon iconData;
			if (length != sizeof(device_icon))
				return B_BAD_VALUE;
			if (user_memcpy(&iconData, buffer, sizeof(device_icon)) != B_OK)
				return B_BAD_ADDRESS;

			if (iconData.icon_size >= (int32)sizeof(kDriveIcon)) {
				if (user_memcpy(iconData.icon_data, kDriveIcon,
						sizeof(kDriveIcon)) != B_OK)
					return B_BAD_ADDRESS;
			}

			iconData.icon_size = sizeof(kDriveIcon);
			return user_memcpy(buffer, &iconData, sizeof(device_icon));
		}

		/*case B_FLUSH_DRIVE_CACHE:
			return synchronize_cache(info);*/
	}

	return B_DEV_INVALID_IOCTL;
}


module_dependency module_dependencies[] = {
	{B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&sDeviceManager},
	{}
};


// The "block device" associated with the device file. It can be open()
// multiple times, eash allocating an mmc_disk_handle. It does not interact
// with the hardware directly, instead it forwards all IO requests to the
// disk driver through the IO scheduler.
struct device_module_info sMMCBlockDevice = {
	{
		MMC_DISK_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	mmc_block_init_device,
	mmc_block_uninit_device,
	NULL, // remove,

	mmc_block_open,
	mmc_block_close,
	mmc_block_free,
	NULL, //mmc_block_read,
	NULL, //mmc_block_write,
	NULL, //mmc_block_io,
	mmc_block_ioctl,

	NULL,	// select
	NULL,	// deselect
};


// Driver for the disk devices itself. This is paired with an
// mmc_disk_driver_info instanciated once per device. Handles the actual disk
// I/O operations
struct driver_module_info sMMCDiskDriver = {
	{
		MMC_DISK_DRIVER_MODULE_NAME,
		0,
		NULL
	},
	mmc_disk_supports_device,
	mmc_disk_register_device,
	mmc_disk_init_driver,
	mmc_disk_uninit_driver,
	mmc_disk_register_child_devices,
	NULL, // mmc_disk_rescan_child_devices,
	NULL,
};


module_info* modules[] = {
	(module_info*)&sMMCDiskDriver,
	(module_info*)&sMMCBlockDevice,
	NULL
};
