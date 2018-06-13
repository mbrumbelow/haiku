#include <new>
#include <stdio.h>
#include <string.h>
#include <KernelExport.h>

#include <PCI_x86.h>
#include <bus/PCI.h>
#include <device_manager.h>

#include "sdhci_pci.h"

#define TRACE_SDHCI
#ifdef TRACE_SDHCI
#	define TRACE(x...) dprintf("\33[33msdhci_pci:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#define TRACE_ALWAYS(x...)	dprintf("\33[33msdhci_pci:\33[0m " x)
#define ERROR(x...)			dprintf("\33[33msdhci_pci:\33[0m " x)
#define CALLED(x...)		TRACE("CALLED %s\n", __PRETTY_FUNCTION__)

#define SDHCI_PCI_DEVICE_MODULE_NAME "busses/mmc/sdhci_pci/driver_v1"
#define SDHCI_PCI_MMC_BUS_MODULE_NAME "busses/mmc/sdhci_pci/device/v1"
#define SDHCI_PCI_CONTROLLER_TYPE_NAME "sdhci pci controller"

typedef struct {
	pci_device_module_info* pci;
	pci_device* device;
	addr_t base_addr;
	uint8 irq;

	device_node* node;
	pci_info info;

} sdhci_pci_mmc_bus_info;

device_manager_info* gDeviceManager;
static pci_x86_module_info* sPCIx86Module;


static void
bus_removed(void* bus_cookie)
{
	return;
}

static void
uninit_bus(void* bus_cookie) {
	sdhci_pci_mmc_bus_info* bus = (sdhci_pci_mmc_bus_info*)bus_cookie;
	delete bus;	
}

static status_t 
init_bus(device_node* node, void** bus_cookie)
{
	CALLED();
	status_t status = B_OK;
	uint16 block_size, block_count, transfer_mode_register, host_control;
	uint32 buffer;
	int32 sample = 2;
	area_id	regs_area;
	vuint8*	regs;
	

	sdhci_pci_mmc_bus_info* bus = new(std::nothrow) sdhci_pci_mmc_bus_info;
	if(bus == NULL)
		return B_NO_MEMORY;

	pci_device_module_info* pci;
	pci_device* device;
	{
		device_node* parent = gDeviceManager->get_parent_node(node);
		device_node* pciParent = gDeviceManager->get_parent_node(parent);
		gDeviceManager->get_driver(pciParent, (driver_module_info**)&pci, (void**)&device);
		gDeviceManager->put_node(pciParent);
		gDeviceManager->put_node(parent);
	}

	if(get_module(B_PCI_X86_MODULE_NAME, (module_info**)&sPCIx86Module) != B_OK)		
		sPCIx86Module = NULL;

	bus->node = node;
	bus->pci = pci;
	bus->device = device;

	pci_info *pciInfo = &bus->info;
	pci->get_pci_info(device, pciInfo);

	bus->base_addr = pciInfo->u.h0.base_registers[0];

	uint16 pcicmd = pci->read_pci_config(device, PCI_command, 2);
	pcicmd &= ~(PCI_command_memory | PCI_command_int_disable);
	pci->write_pci_config(device, PCI_command, 2, pcicmd);

	block_size = bus->pci->read_io_16(bus->device, bus->base_addr + SDHCI_BLOCK_SIZE);
	block_count = bus->pci->read_io_16(bus->device, bus->base_addr + SDHCI_BLOCK_COUNT);

	transfer_mode_register = bus->pci->read_io_16(bus->device, bus->base_addr + 
		SDHCI_TRANSFER_MODE_REGISTER);
	uint32 present_state_register = bus->pci->read_io_32(bus->device, bus->base_addr + 
		SDHCI_PRESENT_STATE_REGISTER);
	int length =  sizeof(pciInfo->u.h0.base_registers);

	TRACE("init_bus() %p node %p pci %p device %p base_addr %p \n"
		, bus, node, bus->pci, bus->device, bus->base_addr);
	
	regs_area = map_physical_memory("sdhc_regs_map",
	pciInfo->u.h0.base_registers[0],
	pciInfo->u.h0.base_register_sizes[0], B_ANY_KERNEL_ADDRESS,
		0, (void**)&regs);	
	if(regs_area < B_OK)
	{
		TRACE("mapping failed");
		return 0.1f;
	}

	transfer_mode_register = *(regs+0);


	host_control = *(regs + 0x3E);

	
	block_count = *(regs + 0x06);

	present_state_register = *(regs + 0x24);

	TRACE("block count enable: %d, data direction select: %d, block_count: %d, multiple block: %d, buf_read: %d,buf_write: %d \n",
		(transfer_mode_register>>1)&1, (transfer_mode_register>>4)&1, block_count, (transfer_mode_register>>5)&1,
		(present_state_register>>11)&1,(present_state_register>>10)&1);


	return status;
}

static status_t
init_device(device_node* node, void** device_cookie)
{
	CALLED();
	*device_cookie = node;
	return B_OK;
}

static status_t
register_child_devices(void* cookie)
{
	CALLED();
	device_node* node = (device_node*)cookie;
	device_node* parent = gDeviceManager-> get_parent_node(node);
	pci_device_module_info *pci;
	pci_device* device; 
	gDeviceManager->get_driver(parent, (driver_module_info**)&pci,(void**)&device);
	uint16 pciSubDeviceId = pci->read_pci_config(device, PCI_subsystem_id, 2);

	char prettyName[25];
	sprintf(prettyName, "SDHC bus %" B_PRIu16, pciSubDeviceId);

	device_attr attrs[] = {

		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE,{string: prettyName}},
//		{B_DEVICE_FIXED_CHILD, B_STRING_TYPE,{string: SDHCI_FOR_CONTROLLER_MODULE_NAME}},		
		{SDHCI_DEVICE_TYPE_ITEM, B_UINT16_TYPE,{ ui16: pciSubDeviceId}},
		{B_DEVICE_BUS, B_STRING_TYPE,{string: "mmc"}},		
		
	//	{SDHCI_VRING_ALLIGNMENT_ITEM, B_UINT16_TYPE, {ui16: SDHCI_PCI_VRING_ALIGN}},
		{NULL}
		
	};
	TRACE("setted up device type, %s, value %d\n",SDHCI_DEVICE_TYPE_ITEM,pciSubDeviceId);
	return gDeviceManager->register_node(node, SDHCI_PCI_MMC_BUS_MODULE_NAME, attrs, NULL, &node);
	
}

static status_t
register_device(device_node* parent)
{
	device_attr attrs[] = {
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {string: "SDHC PCI controller"}},
		{}
	};

	return gDeviceManager->register_node(parent, SDHCI_PCI_DEVICE_MODULE_NAME,
		attrs, NULL, NULL);
}



static float
supports_device(device_node* parent)
{
	CALLED();
	const char* bus;
	uint16 type, subType;
	uint8 bar, pciSlotsInfo, pciSubDeviceId;

	TRACE("Supports device started , success!");
	
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false) != B_OK
	||	gDeviceManager->get_attr_uint16(parent, B_DEVICE_TYPE, &type, false) < B_OK
	|| gDeviceManager->get_attr_uint16(parent, B_DEVICE_SUB_TYPE, &subType, false) < B_OK)
		return -1;
	
	if (strcmp(bus, "pci") != 0) 
		return 0.0f;
	
	if (type == PCI_base_peripheral) { // check for vendor ID
		if (subType != PCI_sd_host) { // check for device ID
			return 0.0f;
		}

		pci_device_module_info* pci;
		pci_device* device;
		gDeviceManager->get_driver(parent, (driver_module_info**)&pci,
			(void**)&device);
		pciSubDeviceId = pci->read_pci_config(device, PCI_revision,
			1);
		pciSlotsInfo = pci->read_pci_config(device, SDHCI_PCI_SLOT_INFO, 1); // second parameter is for offset and third is of reading no of bytes
		bar = SDHCI_PCI_SLOT_INFO_FIRST_BASE_ADDRESS(pciSlotsInfo);
		pciSlotsInfo = SDHCI_PCI_SLOTS(pciSlotsInfo);
		if(pciSlotsInfo > 6 || bar > 5)
		{
			TRACE("Error: slots information");
			return 0.6f;
		}
		TRACE("SDHCI Device found! Subtype: 0x%04x, type: 0x%04x\n", subType, type);
		TRACE("Number of slots: %d first base address: 0%04x\n", pciSlotsInfo, bar);//zero-based numbering 
		return 1.0f;
	}

	return 0;
}

static sdhci_mmc_bus_interface gSDHCIPCIDeviceModule = {
	{
		{
			SDHCI_PCI_MMC_BUS_MODULE_NAME,
			0,
			NULL
		},
		NULL, // supports device
		NULL, //register device
		init_bus,
		uninit_bus,
		NULL, // register child devices
		NULL, // rescan
		bus_removed
	}
};

module_dependency module_dependencies[] = {
	//{ SDHCI_FOR_CONTROLLER_MODULE_NAME, (module_info**)&gSDHCI },
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&gDeviceManager },
	{}
};

static driver_module_info sSDHCIPCIDevice = {
	{
		SDHCI_PCI_DEVICE_MODULE_NAME,
		0,
		NULL
	},
	supports_device,
	register_device,
	init_device,
	NULL,
	register_child_devices,
	NULL,
	NULL,
	NULL
};

module_info* modules[] = { 
	(module_info*)&gSDHCIPCIDeviceModule,
	(module_info* )&sSDHCIPCIDevice,
	NULL
};

