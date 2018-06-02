#ifndef _SDHCI_PCI_H
#define _SDHCI_PCI_H 

#include <device_manager.h>
#include <KernelExport.h>

#define SDHCI_PCI_VENDORID 0x1b36
#define SDHCI_PCI_MIN_DEVICEID 0x0000
#define SDHCI_PCI_MAX_DEVICEID 0x00ff

#define SHDCI_PCI_SLOT_INFO 0x40

typedef void* sdhci_mmc_bus;

typedef struct {
	driver_module_info info;

	void (*set_mmc_bus)(void* cookie, sdhci_mmc_bus mmc_bus);
	status_t (*read_host_features)(void* cookie, uint32* features);
	status_t (*write_guest_features)(void* cookie, uint32 features);
	uint8 (*get_status)(void* cookie);
	void (*set_status)(void* cookie, uint8 status);
	status_t (*read_device_config)(void* cookie, uint8 offset, void* buffer,
		size_t bufferSize);
	status_t (*write_device_config)(void* cookie, uint8 offset,
			const void* buffer, size_t bufferSize);

	uint16 (*get_queue_ring_size)(void* cookie, uint16 queue);
	status_t (*setup_queue)(void* cookie, uint16 queue, phys_addr_t phy);
	status_t (*setup_interrupt)(void* cookie, uint16 queueCount);
	status_t (*free_interrupt)(void* cookie);
	void (*notify_queue)(void* cookie, uint16 queue);	
} sdhci_mmc_bus_interface;

#endif
