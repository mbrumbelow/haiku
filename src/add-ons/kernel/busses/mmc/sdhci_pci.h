
#ifndef _SDHCI_PCI_H
#define _SDHCI_PCI_H 

#include <device_manager.h>
#include <KernelExport.h>

#define SDHCI_PCI_SLOT_INFO 							0x40
#define SDHCI_PCI_SLOTS(x) 								(((x>>4)&7)+1)
#define SDHCI_PCI_SLOT_INFO_FIRST_BASE_ADDRESS(x)		((x) & 7)

#define SDHCI_BLOCK_SIZE 								4
#define SDHCI_BLOCK_COUNT 								6
#define SDHCI_TRANSFER_MODE_REGISTER 					48

#define SDHCI_DEVICE_TYPE_ITEM 							"sdhci/type"
#define SDHCI_BUS_TYPE_NAME 							"bus/sdhci/v1"

#define SDHCI_PRESENT_STATE_REGISTER 					36
#define SDHCI_BUFFER_DATA_PORT_REGISTER 				32



typedef void* sdhci_mmc_bus;

//#define SDHCI_FOR_CONTROLLER_MODULE_NAME "bus_managers/mmc/controller/driver_v1"

typedef struct {
	driver_module_info info;

} sdhci_mmc_bus_interface;



#endif
