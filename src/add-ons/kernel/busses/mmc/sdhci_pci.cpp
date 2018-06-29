/*
 * Copyright 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <new>
#include <stdio.h>
#include <string.h>

#include <bus/PCI.h>
#include <PCI_x86.h>

#include <KernelExport.h>

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

#define SLOTS_COUNT				"device/slots_count"	
#define SLOT_NUMBER				"device/slot"				
#define BAR_INDEX				"device/bar"

struct registers {
	volatile uint32_t system_address;
	volatile uint16_t block_size;
	volatile uint16_t block_count;
	volatile uint32_t argument;
	volatile uint16_t transfer_mode;
	volatile uint16_t command;
	volatile uint32_t response0;
	volatile uint32_t response2;
	volatile uint32_t response4;
	volatile uint32_t response6;
	volatile uint32_t buffer_data_port;
	volatile uint32_t present_state;
	volatile uint8_t power_control;
	volatile uint8_t host_control;
	volatile uint8_t wakeup_control;
	volatile uint8_t block_gap_control;
	volatile uint16_t clock_control;
	volatile uint8_t software_reset;
	volatile uint8_t timeout_control;
	volatile uint16_t normal_interrupt_status;
	volatile uint16_t error_interrupt_status;
	volatile uint16_t normal_interrupt_status_enable;
	volatile uint16_t error_interrupt_status_enable;
	volatile uint16_t normal_interrupt_signal_enable;
	volatile uint16_t error_interrupt_signal_enable;
	volatile uint16_t auto_cmd12_error_status;
	volatile uint32_t : 32;
	volatile uint32_t capabilities;
	volatile uint32_t capabilities_rsvd;
	volatile uint32_t max_current_capabilities;
	volatile uint32_t max_current_capabilities_rsvd;
	volatile uint32_t : 32;
	volatile uint32_t : 32;
	volatile uint32_t : 32;
	volatile uint16_t slot_interrupt_status;
	volatile uint16_t host_control_version;
} __attribute__((packed));


typedef struct {
	pci_device_module_info* pci;
	pci_device* device;
	addr_t base_addr;
	uint8 irq;

	device_node* node;
	pci_info info;

	struct registers* _regs;

} sdhci_pci_mmc_bus_info;


device_manager_info* gDeviceManager;
static pci_x86_module_info* sPCIx86Module;

//	#pragma mark -

static status_t
init_bus(device_node* node, void** bus_cookie)
{
	CALLED();
	status_t status = B_OK;
	area_id	regs_area;
	volatile uint16_t* regs;
	uint8 bar, slot;

	sdhci_pci_mmc_bus_info* bus = new(std::nothrow) sdhci_pci_mmc_bus_info;
	if (bus == NULL) {
		return B_NO_MEMORY;
	}

	pci_info *pciInfo = &bus->info;


	pci_device_module_info* pci;
	pci_device* device;
	{
		device_node* parent = gDeviceManager->get_parent_node(node);
		device_node* pciParent = gDeviceManager->get_parent_node(parent);
		gDeviceManager->get_driver(pciParent, (driver_module_info**)&pci,
			(void**)&device);
		gDeviceManager->put_node(pciParent);
		gDeviceManager->put_node(parent);
	}
	if (get_module(B_PCI_X86_MODULE_NAME, (module_info**)&sPCIx86Module)
			!= B_OK) {
		sPCIx86Module = NULL;
		TRACE("PCIx86Module not loaded\n");
	}	

	int msiCount = sPCIx86Module->get_msi_count(
			pciInfo->bus, pciInfo->device, pciInfo->function);

	TRACE("interrupts count: %d\n",msiCount);

	if(gDeviceManager->get_attr_uint8(node, SLOT_NUMBER, &slot,false) < B_OK
		|| gDeviceManager->get_attr_uint8(node, BAR_INDEX, &bar,false) < B_OK)
	{
		return -1;
	}

	bus->node = node;
	bus->pci = pci;
	bus->device = device;

	pci->get_pci_info(device, pciInfo);

	// legacy interrupt
	bus->base_addr = pciInfo->u.h0.base_registers[bar];

	// enable bus master and io
	uint16 pcicmd = pci->read_pci_config(device, PCI_command, 2);
	//pcicmd &= ~(PCI_command_memory | PCI_command_int_disable);
	// pcicmd |= PCI_command_master | PCI_command_io;
	// pci->write_pci_config(device, PCI_command, 2, pcicmd);

	TRACE("init_bus() %p node %p pci %p device %p\n", bus, node,
		bus->pci, bus->device);

	// mapping the registers by MMUIO method 
	int bar_size = pciInfo->u.h0.base_register_sizes[bar];

	regs_area = map_physical_memory("sdhc_regs_map",
		pciInfo->u.h0.base_registers[bar],
		pciInfo->u.h0.base_register_sizes[bar], B_ANY_KERNEL_BLOCK_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void**)&regs);	

	if(regs_area < B_OK)
	{
		return B_BAD_VALUE;
	}

	struct registers* _regs = (struct registers*)regs;

	TRACE("system_address: %d\n",_regs->system_address);
	TRACE("block_size: %d\n",_regs->block_size);
	TRACE("block_count: %d\n",_regs->block_count);
	TRACE("argument: %d\n",_regs->argument);
	TRACE("transfer_mode: %d\n",_regs->transfer_mode);
	TRACE("command: %d\n",_regs->command);
	TRACE("response0: %d\n",_regs->response0);
	TRACE("response2: %d\n",_regs->response2);
	TRACE("response4: %d\n",_regs->response4);
	TRACE("response6: %d\n",_regs->response6);
	TRACE("buffer_data_port: %d\n",_regs->buffer_data_port);
	TRACE("present_state: %d\n",_regs->present_state);
	TRACE("power_control: %d\n",_regs->power_control);
	TRACE("host_control: %d\n",_regs->host_control);
	TRACE("wakeup_control: %d\n",_regs->wakeup_control);
	TRACE("block_gap_control: %d\n",_regs->block_gap_control);
	TRACE("clock_control: %d\n",_regs->clock_control);
	TRACE("software_reset: %d\n",_regs->software_reset);
	TRACE("timeout_control: %d\n",_regs->timeout_control);
	TRACE("normal_interrupt_status: %d\n",_regs->normal_interrupt_status);
	TRACE("error_interrupt_status: %d\n",_regs->error_interrupt_status);
	TRACE("normal_interrupt_status_enable: %d\n",_regs->normal_interrupt_status_enable);
	TRACE("error_interrupt_status_enable: %d\n",_regs->error_interrupt_status_enable);
	TRACE("normal_interrupt_signal_enable: %d\n",_regs->normal_interrupt_signal_enable);
	TRACE("error_interrupt_signal_enable: %d\n",_regs->error_interrupt_signal_enable);
	TRACE("auto_cmd12_error_status: %d\n",_regs->auto_cmd12_error_status);
	TRACE("capabilities: %d\n",_regs->capabilities);
	TRACE("capabilities_rsvd: %d\n",_regs->capabilities_rsvd);
	TRACE("max_current_capabilities: %d\n",_regs->max_current_capabilities);
	TRACE("max_current_capabilities_rsvd: %d\n",_regs->max_current_capabilities_rsvd);
	TRACE("slot_interrupt_status: %d\n",_regs->slot_interrupt_status);
	TRACE("host_control_version %d\n",_regs->host_control_version);

	TRACE("slots: %d bar: %d  bar_size: %d\n",slot,bar, bar_size);

	bus->_regs = _regs;

	if(regs_area < B_OK)
	{
		TRACE("mapping failed");
		return 0.1f;
	}

	*bus_cookie = bus;
	return status;
}


static void
uninit_bus(void* bus_cookie)
{
	sdhci_pci_mmc_bus_info* bus = (sdhci_pci_mmc_bus_info*)bus_cookie;
	delete bus;
}


static void
bus_removed(void* bus_cookie)
{
	return;
}


//	#pragma mark -

static status_t
register_child_devices(void* cookie)
{
	CALLED();
	device_node* node = (device_node*)cookie;
	device_node* parent = gDeviceManager->get_parent_node(node);
	pci_device_module_info* pci;
	pci_device* device;
	uint8 slots_count, bar, slotsInfo;

	gDeviceManager->get_driver(parent, (driver_module_info**)&pci,
		(void**)&device);

	uint16 pciSubDeviceId = pci->read_pci_config(device, PCI_subsystem_id,
		2);

	slotsInfo = pci->read_pci_config(device, SDHCI_PCI_SLOT_INFO, 1);

	bar = SDHCI_PCI_SLOT_INFO_FIRST_BASE_INDEX(slotsInfo);

	slots_count = SDHCI_PCI_SLOTS(slotsInfo);

	char prettyName[25];

	if(slots_count > 6 || bar > 5)	
	{
		TRACE("Error: slots information");
		return B_BAD_VALUE;
	}

	for(uint8_t slot = 0; slot <= slots_count; slot++)
	{

		bar = bar + slot;

		sprintf(prettyName, "SDHC bus %" B_PRIu16 " slot %" B_PRIu8, pciSubDeviceId, slot);
	
		device_attr attrs[] = {
		// properties of this controller for sdhci bus manager

			{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
				{ string: prettyName }},
			{SDHCI_DEVICE_TYPE_ITEM, B_UINT16_TYPE,
				{ ui16: pciSubDeviceId}},			
			{B_DEVICE_BUS, B_STRING_TYPE,{string: "mmc"}},		
			{SLOT_NUMBER, B_UINT8_TYPE,
				{ ui8: slot}},
			{BAR_INDEX, B_UINT8_TYPE,
				{ ui8: bar}},
			{ NULL }
		};	
		if(gDeviceManager->register_node(node, SDHCI_PCI_MMC_BUS_MODULE_NAME,
			attrs, NULL, &node) != B_OK)
		{
			return B_BAD_VALUE;
		}
	}

	return B_OK;

}


static status_t
init_device(device_node* node, void** device_cookie)
{
	CALLED();
	*device_cookie = node;
	return B_OK;
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
	uint8 pciSubDeviceId;

	// make sure parent is a PCI SDHCI device node
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false) != B_OK
		|| gDeviceManager->get_attr_uint16(parent, B_DEVICE_SUB_TYPE,
				&subType, false) < B_OK
		|| gDeviceManager->get_attr_uint16(parent, B_DEVICE_TYPE, &type,
				false) < B_OK) {
		return -1;
	}

	if (strcmp(bus, "pci") != 0)
		return 0.0f;

	if (type == PCI_base_peripheral) {
		if (subType != PCI_sd_host) {
			return 0.0f;
		}

		pci_device_module_info* pci;
		pci_device* device;
		gDeviceManager->get_driver(parent, (driver_module_info**)&pci,
			(void**)&device);

		pciSubDeviceId = pci->read_pci_config(device, PCI_revision,
			1);
		TRACE("SDHCI Device found! Subtype: 0x%04x, type: 0x%04x\n",
			subType, type);
		return 0.8f;
	}

	return 0.0f;
}


//	#pragma mark -

module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&gDeviceManager },
	{}
};


static sdhci_mmc_bus_interface gSDHCIPCIDeviceModule = {
	{
		{
			SDHCI_PCI_MMC_BUS_MODULE_NAME,
			0,
			NULL
		},

		NULL,	// supports device
		NULL,	// register device
		init_bus,
		uninit_bus,
		NULL,	// register child devices
		NULL,	// rescan
		bus_removed,

	}
};


static driver_module_info sSDHCIDevice = {
	{
		SDHCI_PCI_DEVICE_MODULE_NAME,
		0,
		NULL
	},
	supports_device,
	register_device,
	init_device,
	NULL,	// uninit
	register_child_devices,
	NULL,	// rescan
	NULL,	// device removed
};

module_info* modules[] = {
	(module_info* )&sSDHCIDevice,
	(module_info* )&gSDHCIPCIDeviceModule,
	NULL
};
