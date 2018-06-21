#ifndef _MMC_DISK_H
#define _MMC_DISK_H 

#include <device_manager.h>
#include <KernelExport.h>

#define SDHCI_DEVICE_TYPE_ITEM "sdhci/type"

typedef struct {

	device_node*			node;

	status_t media_status;

} mmc_disk_driver_info;

#endif