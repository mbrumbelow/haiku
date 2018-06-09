
#ifndef _SDHCI_PCI_H
#define _SDHCI_PCI_H 

#include <device_manager.h>
#include <KernelExport.h>

#define SDHCI_PCI_VENDORID 0x1b36
#define SDHCI_PCI_MIN_DEVICEID 0x0000
#define SDHCI_PCI_MAX_DEVICEID 0x00ff

#define SHDCI_PCI_SLOT_INFO 0x40
#define SDHCI_BLOCK_SIZE 4
#define SDHCI_BLOCK_COUNT 6
#define SDHCI_TRANSFER_MODE_REGISTER 48

#define SDHCI_DEVICE_TYPE_ITEM "sdhci/type"
#define SDHCI_BUS_TYPE_NAME "bus/sdhci/v1"




typedef void* sdhci_mmc_bus;

#define SDHCI_FOR_CONTROLLER_MODULE_NAME "bus_managers/mmc/controller/driver_v1"

typedef struct {
	driver_module_info info;
	void (*map_registers)(pci_info *pciInfo);

} sdhci_mmc_bus_interface;

#endif
