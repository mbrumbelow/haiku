#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "mmc_disk.h"

#include <drivers/device_manager.h>
#include <drivers/KernelExport.h>
#include <drivers/Drivers.h>
#include <kernel/OS.h>

#include <fs/devfs.h>


#define TRACE_MMC_DISK
#ifdef TRACE_MMC_DISK
#	define TRACE(x...) dprintf("mmc_disk: " x)
#else
#	define TRACE(x...) ;
#endif
#define ERROR(x...)			dprintf("\33[33mmc_disk:\33[0m " x)
#define CALLED() 			TRACE("CALLED %s\n", __PRETTY_FUNCTION__)

#define MMC_DISK_DRIVER_MODULE_NAME "drivers/disk/mmc/mmc_disk/driver_v1"

static device_manager_info *sDeviceManager;

static float
mmc_disk_supports_device(device_node *parent)
{
	CALLED();
	const char *bus;
	uint16 deviceType;

	if(sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return -1;

	if(strcmp(bus, "pci"))
		return 0.0;
	if(sDeviceManager->get_attr_uint16(parent, SDHCI_DEVICE_TYPE_ITEM,
		&deviceType, true) != B_OK)
		return 0.0;
	
	TRACE("sdhci device found");

	return 0.8;

}

// static status_t mmc_disk_register_device(device_node *node){}

// static status_t mmc_disk_init_driver(device_node*, void **cookie){}

// static void mmc_disk_uninit_driver(void *_cookie){}

// static status_t mmc_disk_register_child_devices(void *_cookie){}


module_dependency module_dependencies[] = {
	{B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&sDeviceManager},
	{}
};

struct driver_module_info sMMCDiskDriver = {
	{
		MMC_DISK_DRIVER_MODULE_NAME,
		0,
		NULL
	},
	mmc_disk_supports_device,
	NULL, //mmc_disk_register_device,
	NULL, // mmc_disk_init_driver,
	NULL, //mmc_disk_uninit_driver,
	NULL, //mmc_disk_register_child_devices,
	NULL, //mmc_disk_rescan_child_devices,
	NULL,	
};

module_info* modules[] = {
	(module_info*)&sMMCDiskDriver,
	NULL
};
