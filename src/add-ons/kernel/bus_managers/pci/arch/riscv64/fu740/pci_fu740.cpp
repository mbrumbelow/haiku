/*
 * Copyright 2009-2020, Haiku Inc., All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "pci_fu740.h"


status_t
PCIFU740::Init(fdt_device_module_info* parentModule);
{
	if (parentModule == NULL)
		return B_ERROR;

	fdt_device* parentDev;
	if (gDeviceManager->get_driver(parent.Get(),
		(driver_module_info**)&parentModule, (void**)&parentDev))
		panic("can't get parent node driver");

	uint64 regs;
	uint64 regsLen;

	for (uint32 i = 0; parentModule->get_reg(parentDev, i, &regs, &regsLen);
		i++) {
		dprintf("  reg[%" B_PRIu32 "]: (0x%" B_PRIx64 ", 0x%" B_PRIx64 ")\n",
			i, regs, regsLen);
	}

	uint64 configRegs = 0;
	uint64 configRegsLen = 0;
	uint64 dbiRegs = 0;
	uint64 dbiRegsLen = 0;
	if (!parentModule->get_reg(parentDev, 0, &dbiRegs, &dbiRegsLen)
		|| !parentModule->get_reg(parentDev, 1, &configRegs, &configRegsLen)) {
				dprintf("  no regs\n");
				return B_ERROR;
	}
/*
			// !!!
			configRegs = 0x60070000;
			configRegsLen = 0x10000;
*/

	dprintf("  configRegs: (0x%" B_PRIx64 ", 0x%" B_PRIx64 ")\n",
		configRegs, configRegsLen);
	dprintf("  dbiRegs: (0x%" B_PRIx64 ", 0x%" B_PRIx64 ")\n",
		dbiRegs, dbiRegsLen);

	int intMapLen;
	const void* intMapAdr = parentModule->get_prop(parentDev, "interrupt-map",
		&intMapLen);
	if (intMapAdr == NULL) {
		dprintf("  \"interrupt-map\" property not found");
		return B_ERROR;
	} else {
		int intMapMaskLen;
		const void* intMapMask = parentModule->get_prop(parentDev, "interrupt-map-mask",
			&intMapMaskLen);

		if (intMapMask == NULL || intMapMaskLen != 4 * 4) {
			dprintf("  \"interrupt-map-mask\" property not found or invalid");
			return B_ERROR;
		}

		fInterruptMapMask.childAdr = B_BENDIAN_TO_HOST_INT32(*((uint32*)intMapMask + 0));
		fInterruptMapMask.childIrq = B_BENDIAN_TO_HOST_INT32(*((uint32*)intMapMask + 3));

		sInterruptMapLen = (uint32)intMapLen / (6 * 4);
		sInterruptMap.SetTo(new(std::nothrow) InterruptMap[sInterruptMapLen]);
		if (!sInterruptMap.IsSet())
			return B_NO_MEMORY;

		for (uint32_t *it = (uint32_t*)intMapAdr;
			(uint8_t*)it - (uint8_t*)intMapAdr < intMapLen; it += 6) {
			size_t i = (it - (uint32_t*)intMapAdr) / 6;

			sInterruptMap[i].childAdr = B_BENDIAN_TO_HOST_INT32(*(it + 0));
			sInterruptMap[i].childIrq = B_BENDIAN_TO_HOST_INT32(*(it + 3));
			sInterruptMap[i].parentIrqCtrl = B_BENDIAN_TO_HOST_INT32(*(it + 4));
			sInterruptMap[i].parentIrq = B_BENDIAN_TO_HOST_INT32(*(it + 5));
		}

		dprintf("  interrupt-map:\n");
		for (size_t i = 0; i < sInterruptMapLen; i++) {
			dprintf("    ");
			// child unit address
			uint8 bus, device, function;
			DecodePciAddress(sInterruptMap[i].childAdr, bus, device, function);
			dprintf("bus: %" B_PRIu32, bus);
			dprintf(", dev: %" B_PRIu32, device);
			dprintf(", fn: %" B_PRIu32, function);

			dprintf(", childIrq: %" B_PRIu32,
				sInterruptMap[i].childIrq);
			dprintf(", parentIrq: (%" B_PRIu32,
				sInterruptMap[i].parentIrqCtrl);
			dprintf(", %" B_PRIu32, sInterruptMap[i].parentIrq);
			dprintf(")\n");
			if (i % 4 == 3 && (i + 1 < sInterruptMapLen))
				dprintf("\n");
		}
	}

	memset(fRegisterRanges, 0, sizeof(fRegisterRanges));
	int rangesLen;
	const void* rangesAdr = parentModule->get_prop(parentDev, "ranges",
		&rangesLen);
	if (rangesAdr == NULL) {
		dprintf("  \"ranges\" property not found");
	} else {
		dprintf("  ranges:\n");
		for (uint32_t *it = (uint32_t*)rangesAdr;
			(uint8_t*)it - (uint8_t*)rangesAdr < rangesLen; it += 7) {
			dprintf("    ");
			uint32_t kind = B_BENDIAN_TO_HOST_INT32(*(it + 0));
			uint64_t childAdr = B_BENDIAN_TO_HOST_INT64(*(uint64_t*)(it + 1));
			uint64_t parentAdr = B_BENDIAN_TO_HOST_INT64(*(uint64_t*)(it + 3));
			uint64_t len = B_BENDIAN_TO_HOST_INT64(*(uint64_t*)(it + 5));

			switch (kind & 0x03000000) {
			case 0x01000000:
				SetRegisterRange(kRegIo, parentAdr, childAdr, len);
				break;
			case 0x02000000:
				SetRegisterRange(kRegMmio32, parentAdr, childAdr, len);
				break;
			case 0x03000000:
				SetRegisterRange(kRegMmio64, parentAdr, childAdr, len);
				break;
			}

			switch (kind & 0x03000000) {
			case 0x00000000: dprintf("CONFIG"); break;
			case 0x01000000: dprintf("IOPORT"); break;
			case 0x02000000: dprintf("MMIO32"); break;
			case 0x03000000: dprintf("MMIO64"); break;
			}

			dprintf(" (0x%08" B_PRIx32 "): ", kind);
			dprintf("child: %08" B_PRIx64, childAdr);
			dprintf(", parent: %08" B_PRIx64, parentAdr);
			dprintf(", len: %" B_PRIx64 "\n", len);
		}
	}

	fConfigPhysBase = configRegs;
	fConfigSize = configRegsLen;
	fConfigArea.SetTo(map_physical_memory("PCI Config MMIO", configRegs,
		fConfigSize, B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		(void**)&fConfigBase));

	if (dbiRegs != 0) {
		fDbiPhysBase = dbiRegs;
		fDbiSize = dbiRegsLen;
		fDbiArea.SetTo(map_physical_memory("PCI DBI MMIO", dbiRegs, fDbiSize,
			B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
			(void**)&fDbiBase));
	}

	fIoArea.SetTo(map_physical_memory("PCI IO", fRegisterRanges[kRegIo].parentBase,
		fRegisterRanges[kRegIo].size, B_ANY_KERNEL_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void**)&fIoBase));

	if (!fConfigArea.IsSet()) {
		dprintf("  can't map Config MMIO\n");
		return fConfigArea.Get();
	}

	if (!fIoArea.IsSet()) {
		dprintf("  can't map IO\n");
		return fIoArea.Get();
	}

	DumpAtu();

	// TODO read MSI IRQ from FDT
	InitPciMsi(0x38);
	AllocRegs();

	DumpAtu();

	return B_OK;
}


addr_t
PCIFU740::ConfigAddress(uint8 bus, uint8 device, uint8 function, uint16 offset)
{
	uint32 atuType;
	if (bus == 0) {
		if (device != 0 || function != 0)
			return 0;
		return gPCIeDbiBase + offset;
	} else if (bus == 1)
		atuType = kPciAtuTypeCfg0;
	else
		atuType = kPciAtuTypeCfg1;

	status_t res = AtuMap(1, kPciAtuOutbound, atuType, gPCIeConfigPhysBase, EncodePciAddress(bus, device, function) << 8, gPCIeConfigSize);
	if (res < B_OK)
		return 0;

	return fConfigBase + offset;
}


void
PCIFU740::InitDeviceMSI(uint8 bus, uint8 device, uint8 function)
{
	uint32 status;
	uint32 capPtr;
	uint32 capId;

	ReadConfig(NULL, bus, device, function, PCI_status, 2, &status);
	if ((status & PCI_status_capabilities) == 0)
		return;

	uint32 headerType;
	ReadConfig(NULL, bus, device, function, PCI_header_type, 1, &headerType);

	switch (headerType & PCI_header_type_mask) {
		case PCI_header_type_generic:
		case PCI_header_type_PCI_to_PCI_bridge:
			ReadConfig(NULL, bus, device, function, PCI_capabilities_ptr, 1, &capPtr);
			break;
		case PCI_header_type_cardbus:
			ReadConfig(NULL, bus, device, function, PCI_capabilities_ptr_2, 1, &capPtr);
			break;
		default:
			return;
	}

	capPtr &= ~3;
	if (capPtr == 0)
		return;

	for (int i = 0; i < 48; i++) {
		ReadConfig(NULL, bus, device, function, capPtr + 0, 1, &capId);

		if (capId == PCI_cap_id_msi)
			break;

		ReadConfig(NULL, bus, device, function, capPtr + 1, 1, &capPtr);
		capPtr &= ~3;
		if (capPtr == 0)
			return;
	}

	dprintf("  MSI offset: %#x\n", capPtr);

	int32 msiIrq = AllocMsiInterrupt();
	dprintf("  msiIrq: %" B_PRId32 "\n", msiIrq);

	uint32 control;
	ReadConfig(NULL, bus, device, function, capPtr + PCI_msi_control, 2, &control);

	WriteConfig(NULL, bus, device, function, capPtr + PCI_msi_address, 4, (uint32)fMsiPhysAddr);
	if ((control & PCI_msi_control_64bit) != 0) {
		WriteConfig(NULL, bus, device, function, capPtr + PCI_msi_address_high, 4, (uint32)(gMsiPhysAddr >> 32));
		WriteConfig(NULL, bus, device, function, capPtr + PCI_msi_data_64bit, 2, msiIrq);
	} else
		WriteConfig(NULL, bus, device, function, capPtr + PCI_msi_data, 2, msiIrq);

	control &= ~PCI_msi_control_mme_mask;
	control |= (ffs(1) - 1) << 4;
	control |= PCI_msi_control_enable;
	WriteConfig(NULL, bus, device, function, capPtr + PCI_msi_control, 2, control);
	WriteConfig(NULL, bus, device, function, PCI_interrupt_line, 1, gStartMsiIrq + msiIrq);
}
