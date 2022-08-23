/*
 * Copyright 2009-2022, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */


#include "pci_controller.h"

#include <kernel/debug.h>
#include <kernel/int.h>
#include <util/Vector.h>

#include <AutoDeleterDrivers.h>

#include "pci_private.h"
#include "pci.h"

#include <ACPI.h> // module
#include <acpi.h>
#include <drivers/bus/FDT.h>

#include "acpi_irq_routing_table.h"


addr_t gPCIeBase;
uint8 gStartBusNumber;
uint8 gEndBusNumber;
addr_t gPCIioBase;

extern pci_controller pci_controller_ecam;


enum RangeType
{
	RANGE_IO,
	RANGE_MEM
};


struct PciRange
{
	RangeType type;
	phys_addr_t hostAddr;
	phys_addr_t pciAddr;
	size_t length;
};


static Vector<PciRange> *sRanges;


template<typename T>
acpi_address64_attribute AcpiCopyAddressAttr(const T &src)
{
	acpi_address64_attribute dst;

	dst.granularity = src.granularity;
	dst.minimum = src.minimum;
	dst.maximum = src.maximum;
	dst.translation_offset = src.translation_offset;
	dst.address_length = src.address_length;

	return dst;
}


static acpi_status
AcpiCrsScanCallback(acpi_resource *res, void *context)
{
	Vector<PciRange> &ranges = *(Vector<PciRange>*)context;

	acpi_address64_attribute address;
	if (res->type == ACPI_RESOURCE_TYPE_ADDRESS16)
		address = AcpiCopyAddressAttr(res->data.address16.address);
	else if (res->type == ACPI_RESOURCE_TYPE_ADDRESS32)
		address = AcpiCopyAddressAttr(res->data.address32.address);
	else if (res->type == ACPI_RESOURCE_TYPE_ADDRESS64)
		address = AcpiCopyAddressAttr(res->data.address64.address);
	else
		return B_OK;

	acpi_resource_address &common = res->data.address;

	if (common.resource_type != 0 && common.resource_type != 1)
		return B_OK;

	ASSERT(address.minimum + address.address_length - 1 == address.maximum);

	PciRange range;
	range.type = (common.resource_type == 0 ? RANGE_MEM : RANGE_IO);
	range.hostAddr = address.minimum + address.translation_offset;
	range.pciAddr = address.minimum;
	range.length = address.address_length;
	ranges.PushBack(range);

	return B_OK;
}


static bool
is_interrupt_available(int32 gsi)
{
	return true;
}


status_t
pci_controller_init(void)
{
	if (gPCIRootNode == NULL)
		return B_DEV_NOT_READY;

	dprintf("PCI: pci_controller_init\n");

	DeviceNodePutter<&gDeviceManager>
		parent(gDeviceManager->get_parent_node(gPCIRootNode));

	if (parent.Get() == NULL)
		return B_ERROR;

	const char *bus;
	if (gDeviceManager->get_attr_string(parent.Get(), B_DEVICE_BUS, &bus, false) < B_OK)
		return B_ERROR;

	if (strcmp(bus, "fdt") == 0) {
		dprintf("initialize PCI controller from FDT\n");

		status_t res;
		fdt_device_module_info* parentModule;
		fdt_device* parentDev;

		res = gDeviceManager->get_driver(parent.Get(),
			(driver_module_info**)&parentModule, (void**)&parentDev);
		if (res != B_OK) {
			dprintf("can't get parent node driver\n");
			return B_ERROR;
		}

		uint64 configRegs = 0;
		uint64 configRegsLen = 0;

		parentModule->get_reg(parentDev, 0, &configRegs, &configRegsLen);
		dprintf("  configRegs: (0x%" B_PRIx64 ", 0x%" B_PRIx64 ")\n",
			configRegs, configRegsLen);

		area_id area = map_physical_memory("pci config",
			configRegs, configRegsLen, B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void **)&gPCIeBase);

		if (area < 0)
			return B_ERROR;

		sRanges = new Vector<PciRange>();
		int rangesLen;
		const void* rangesAddr = parentModule->get_prop(parentDev, "ranges",
			&rangesLen);
		if (rangesAddr == NULL) {
			dprintf("  ranges property not found\n");
			return B_ERROR;
		}

		for (uint32_t *it = (uint32_t*)rangesAddr;
				(uint8_t*)it - (uint8_t*)rangesAddr < rangesLen; it += 7) {
			uint32_t kind = B_BENDIAN_TO_HOST_INT32(*(it + 0));
			uint64_t childAddr = B_BENDIAN_TO_HOST_INT64(*(uint64_t*)(it + 1));
			uint64_t parentAddr = B_BENDIAN_TO_HOST_INT64(*(uint64_t*)(it + 3));
			uint64_t len = B_BENDIAN_TO_HOST_INT64(*(uint64_t*)(it + 5));

			PciRange range;
			range.type = ((kind & 0x03000000) == 0x01000000) ? RANGE_IO : RANGE_MEM;
			range.hostAddr = parentAddr;
			range.pciAddr = childAddr;
			range.length = len;
			sRanges->PushBack(range);

			if ((kind & 0x03000000) != 0x01000000)
				continue;

			dprintf(" (0x%08" B_PRIx32 "): ", kind);
			dprintf("child: %08" B_PRIx64, childAddr);
			dprintf(", parent: %08" B_PRIx64, parentAddr);
			dprintf(", len: %" B_PRIx64 "\n", len);

			area_id area = map_physical_memory("pci io",
				parentAddr, len, B_ANY_KERNEL_ADDRESS,
				B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void **)&gPCIioBase);

			if (area < 0)
				return B_ERROR;
		}

		gStartBusNumber = 0;
		gEndBusNumber = 0xff;

		return pci_controller_add(&pci_controller_ecam, NULL);
	}

	if (strcmp(bus, "acpi") == 0) {
		dprintf("initialize PCI controller from ACPI\n");

		status_t res;
		acpi_module_info *acpiModule;
		acpi_device_module_info *acpiDeviceModule;
		acpi_device acpiDevice;

		res = get_module(B_ACPI_MODULE_NAME, (module_info**)&acpiModule);
		if (res != B_OK)
			return B_ERROR;

		acpi_mcfg *mcfg;
		res = acpiModule->get_table(ACPI_MCFG_SIGNATURE, 0, (void**)&mcfg);
		if (res != B_OK)
			return B_ERROR;

		res = gDeviceManager->get_driver(parent.Get(),
			(driver_module_info**)&acpiDeviceModule, (void**)&acpiDevice);
		if (res != B_OK)
			return B_ERROR;

		sRanges = new Vector<PciRange>();

		acpi_status acpi_res = acpiDeviceModule->walk_resources(acpiDevice,
			(char *)"_CRS", AcpiCrsScanCallback, sRanges);

		if (acpi_res != 0)
			return B_ERROR;

		for (Vector<PciRange>::Iterator it = sRanges->Begin(); it != sRanges->End(); it++) {
			if (it->type != RANGE_IO)
				continue;

			if (gPCIioBase != 0) {
				dprintf("PCI: multiple io ranges not supported!");
				continue;
			}

			area_id area = map_physical_memory("pci io",
				it->hostAddr, it->length, B_ANY_KERNEL_ADDRESS,
				B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void **)&gPCIioBase);

			if (area < 0)
				return B_ERROR;
		}

		acpi_mcfg_allocation *end = (acpi_mcfg_allocation *) ((char*)mcfg + mcfg->header.length);
		acpi_mcfg_allocation *alloc = (acpi_mcfg_allocation *) (mcfg + 1);

		if (alloc + 1 != end)
			dprintf("PCI: multiple host bridges not supported!");

		for (; alloc < end; alloc++) {
			dprintf("PCI: mechanism addr: %" B_PRIx64 ", seg: %x, start: %x, end: %x\n",
				alloc->address, alloc->pci_segment, alloc->start_bus_number, alloc->end_bus_number);

			if (alloc->pci_segment != 0) {
				dprintf("PCI: multiple segments not supported!");
				continue;
			}

			gStartBusNumber = alloc->start_bus_number;
			gEndBusNumber = alloc->end_bus_number;

			area_id area = map_physical_memory("pci config",
				alloc->address, (gEndBusNumber - gStartBusNumber + 1) << 20, B_ANY_KERNEL_ADDRESS,
				B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void **)&gPCIeBase);

			if (area < 0)
				break;

			dprintf("PCI: ecam controller found\n");
			return pci_controller_add(&pci_controller_ecam, NULL);
		}
	}

	return B_ERROR;
}


static void
pci_controller_finalize_interrupts(fdt_device_module_info* fdtModule, struct fdt_interrupt_map* interruptMap,
	int bus, int device, int function)
{
	uint32 childAddr = ((bus & 0xff) << 16) | ((device & 0x1f) << 11) | ((function & 0x07) << 8);
	uint32 interruptPin = pci_read_config(bus, device, function, PCI_interrupt_pin, 1);

	if (interruptPin == 0xffffffff) {
		dprintf("Error: Unable to read interrupt pin!\n");
		return;
	}

	uint32 irq = fdtModule->lookup_interrupt_map(interruptMap, childAddr, interruptPin);
	if (irq == 0xffffffff) {
		dprintf("no interrupt mapping for childAddr: (%d:%d:%d), childIrq: %d)\n",
			bus, device, function, interruptPin);
	} else {
		dprintf("configure interrupt (%d,%d,%d) --> %d\n",
			bus, device, function, irq);
		pci_update_interrupt_line(bus, device, function, irq);
	}
}


status_t
pci_controller_finalize(void)
{
	DeviceNodePutter<&gDeviceManager>
		parent(gDeviceManager->get_parent_node(gPCIRootNode));

	if (parent.Get() == NULL)
		return B_ERROR;

	const char* bus;
	if (gDeviceManager->get_attr_string(parent.Get(), B_DEVICE_BUS, &bus, false) < B_OK)
		return B_ERROR;

	if (strcmp(bus, "fdt") == 0) {
		dprintf("finalize PCI controller from FDT\n");

		status_t res;
		fdt_device_module_info* parentModule;
		fdt_device* parentDev;

		res = gDeviceManager->get_driver(parent.Get(),
			(driver_module_info**)&parentModule, (void**)&parentDev);
		if (res != B_OK) {
			dprintf("can't get parent node driver\n");
			return B_ERROR;
		}

		struct fdt_interrupt_map* interruptMap = parentModule->get_interrupt_map(parentDev);
		parentModule->print_interrupt_map(interruptMap);

		for (int bus = 0; bus < 8; bus++) {
			for (int device = 0; device < 32; device++) {
				uint32 vendorID = pci_read_config(bus, device, 0, PCI_vendor_id, 2);
				if ((vendorID != 0xffffffff) && (vendorID != 0xffff)) {
					uint32 headerType = pci_read_config(bus, device, 0, PCI_header_type, 1);
					if ((headerType & 0x80) != 0) {
						for (int function = 0; function < 8; function++) {
							pci_controller_finalize_interrupts(parentModule, interruptMap, bus, device, function);
						}
					} else {
						pci_controller_finalize_interrupts(parentModule, interruptMap, bus, device, 0);
					}
				}
			}
		}

		return B_OK;
	}

	if (strcmp(bus, "acpi") == 0) {
		dprintf("finalize PCI controller from ACPI\n");

		status_t res;

		acpi_module_info *acpiModule;
		res = get_module(B_ACPI_MODULE_NAME, (module_info**)&acpiModule);
		if (res != B_OK)
			return B_ERROR;

		IRQRoutingTable table;
		res = prepare_irq_routing(acpiModule, table, &is_interrupt_available);
		if (res != B_OK) {
			dprintf("PCI: irq routing preparation failed\n");
			return B_ERROR;
		}

		res = enable_irq_routing(acpiModule, table);
		if (res != B_OK) {
			dprintf("PCI: irq routing failed\n");
			return B_ERROR;
		}

		print_irq_routing_table(table);

		return B_OK;
	}

	return B_ERROR;
}


phys_addr_t
pci_ram_address(phys_addr_t addr)
{
	for (Vector<PciRange>::Iterator it = sRanges->Begin(); it != sRanges->End(); it++) {
		if (addr >= it->pciAddr && addr < it->pciAddr + it->length) {
			phys_addr_t result = addr - it->pciAddr;
			if (it->type != RANGE_IO)
				result += it->hostAddr;
			return result;
		}
	}

	dprintf("PCI: requested translation of invalid address %" B_PRIxPHYSADDR "\n", addr);
	return 0;
}
