/*
 * Copyright 2009-2020, Haiku Inc., All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "arch_pci_controller.h"

#include "pci_controller_private.h"
#include "pci_msi.h"

#include <util/AutoLock.h>

#include "pci_private.h"

#include "fu740/pci_fu740.h"
#include "ecam/pci_ecam.h"


extern PCI* gPCI;
ArchPCIController* gArchPCI = NULL;


#if 0
static status_t
AtuMap(uint32 index, uint32 direction, uint32 type, uint64 parentAdr, uint64 childAdr, uint32 size)
{
/*
	dprintf("AtuMap(%" B_PRIu32 ", %" B_PRIu32 ", %#" B_PRIx64 ", %#" B_PRIx64 ", %#" B_PRIx32 ")\n",
		index, type, parentAdr, childAdr, size
	);
*/
	volatile PciAtuRegs* atu = (PciAtuRegs*)(fDbiBase + kPciAtuOffset + (2*index + direction)*sizeof(PciAtuRegs));
	atu->baseLo = (uint32)parentAdr;
	atu->baseHi = (uint32)(parentAdr >> 32);
	atu->limit = (uint32)(parentAdr + size - 1);
	atu->targetLo = (uint32)childAdr;
	atu->targetHi = (uint32)(childAdr >> 32);
	atu->ctrl1 = type;
	atu->ctrl2 = kPciAtuEnable;

	for (;;) {
		if ((atu->ctrl2 & kPciAtuEnable) != 0)
			break;
	}

	return B_OK;
}
#endif


static uint32_t
EncodePciAddress(uint8 bus, uint8 device, uint8 function)
{
	return bus % (1 << 8) * (1 << 16)
		+ device % (1 << 5) * (1 << 11)
		+ function % (1 << 3) * (1 << 8);
}


static void
DecodePciAddress(uint32_t adr, uint8& bus, uint8& device, uint8& function)
{
	bus = adr / (1 << 16) % (1 << 8);
	device = adr / (1 << 11) % (1 << 5);
	function = adr / (1 << 8) % (1 << 3);
}


ArchPCIController::ArchPCIController()
	:
	fHostCtrlType(0),
	fConfigPhysBase(0),
	fConfigBase(0),
	fConfigSize(0),
	fDbiPhysBase(0),
	fDbiBase(0),
	fDbiSize(0),
	fIoBase(0),
	fInterruptMapLen(0)
{
}


ArchPCIController::~ArchPCIController()
{
}


RegisterRange*
ArchPCIController::GetRegisterRange(int kind)
{
	if (kind > 3)
		return NULL;

	return &fRegisterRanges[kind];
}


void
ArchPCIController::SetRegisterRange(int kind, phys_addr_t parentBase, phys_addr_t childBase,
	size_t size)
{
	auto& range = fRegisterRanges[kind];

	range.parentBase = parentBase;
	range.childBase = childBase;
	range.size = size;
	// Avoid allocating zero address.
	range.free = (childBase != 0) ? childBase : 1;
}


phys_addr_t
ArchPCIController::AllocRegister(int kind, size_t size)
{
	auto& range = fRegisterRanges[kind];

	phys_addr_t adr = ROUNDUP(range.free, size);
	if (adr - range.childBase + size > range.size)
		return 0;

	range.free = adr + size;

	return adr;
}


InterruptMap*
ArchPCIController::LookupInterruptMap(uint32_t childAdr, uint32_t childIrq)
{
	childAdr &= fInterruptMapMask.childAdr;
	childIrq &= fInterruptMapMask.childIrq;
	for (uint32 i = 0; i < fInterruptMapLen; i++) {
		if ((fInterruptMap[i].childAdr) == childAdr
			&& (fInterruptMap[i].childIrq) == childIrq)
			return &fInterruptMap[i];
	}
	return NULL;
}


void
ArchPCIController::AllocRegs()
{
	dprintf("AllocRegs()\n");
	// TODO: improve enumeration
	for (int j = 0; j < 8; j++) {
		for (int i = 0; i < 32; i++) {
			uint32 vendorID;
			status_t res = ReadConfig(NULL, j, i, 0, PCI_vendor_id, 2, &vendorID);
			if (res >= B_OK && vendorID != 0xffff) {
				uint32 headerType = 0;
				ReadConfig(NULL, j, i, 0,
					PCI_header_type, 1, &headerType);
				if ((headerType & 0x80) != 0) {
					for (int k = 0; k < 8; k++)
						AllocRegsForDevice(j, i, k);
				} else
					AllocRegsForDevice(j, i, 0);
			}
		}
	}
}


status_t
ArchPCIController::ReadConfig(void *cookie, uint8 bus, uint8 device, uint8 function,
	uint16 offset, uint8 size, uint32 *value)
{
	addr_t address = ConfigAddress(bus, device, function, offset);
	if (address == 0)
		return B_ERROR;

	switch (size) {
		case 1:
			*value = *(uint8*)address;
			break;
		case 2:
			*value = *(uint16*)address;
			break;
		case 4:
			*value = *(uint32*)address;
			break;
		default:
			return B_ERROR;
	}
	return B_OK;
}


status_t
ArchPCIController::WriteConfig(void *cookie, uint8 bus, uint8 device, uint8 function,
	uint16 offset, uint8 size, uint32 value)
{
	addr_t address = ConfigAddress(bus, device, function, offset);

	if (address == 0)
		return B_ERROR;

	switch (size) {
		case 1:
			*(uint8*)address = value;
			break;
		case 2:
			*(uint16*)address = value;
			break;
		case 4:
			*(uint32*)address = value;
			break;
		default:
			return B_ERROR;
	}

	return B_OK;
}


void
ArchPCIController::DumpAtu()
{
	dprintf("ATU:\n");
	for (uint32 direction = 0; direction < 2; direction++) {
		switch (direction) {
			case kPciAtuOutbound:
				dprintf("  outbound:\n");
				break;
			case kPciAtuInbound:
				dprintf("  inbound:\n");
				break;
		}

		for (uint32 index = 0; index < 8; index++) {
			volatile PciAtuRegs* atu = (PciAtuRegs*)(fDbiBase
				+ kPciAtuOffset + (2 * index + direction) * sizeof(PciAtuRegs));

			dprintf("    %" B_PRIu32 ": ", index);
			dprintf("base: %#08" B_PRIx64, atu->baseLo + ((uint64)atu->baseHi << 32));
			dprintf(", limit: %#08" B_PRIx32, atu->limit);
			dprintf(", target: %#08" B_PRIx64, atu->targetLo
				+ ((uint64)atu->targetHi << 32));
			dprintf(", ctrl1: ");
			uint32 ctrl1 = atu->ctrl1;
			switch (ctrl1) {
				case kPciAtuTypeMem:
					dprintf("mem");
					break;
				case kPciAtuTypeIo:
					dprintf("io");
					break;
				case kPciAtuTypeCfg0:
					dprintf("cfg0");
					break;
				case kPciAtuTypeCfg1:
					dprintf("cfg1");
					break;
				default:
					dprintf("? (%#" B_PRIx32 ")", ctrl1);
			}
			dprintf(", ctrl2: {");
			uint32 ctrl2 = atu->ctrl2;
			bool first = true;
			for (uint32 i = 0; i < 32; i++) {
				if (((1 << i) & ctrl2) != 0) {
					if (first)
						first = false;
					else
						dprintf(", ");
					switch (i) {
						case 30:
							dprintf("barModeEnable");
							break;
						case 31:
							dprintf("enable");
							break;
						default:
							dprintf("? (%" B_PRIu32 ")", i);
							break;
					}
				}
			}
			dprintf("}\n");
		}
	}
}


void
ArchPCIController::AllocRegsForDevice(uint8 bus, uint8 device, uint8 function)
{
	dprintf("AllocRegsForDevice(bus: %d, device: %d, function: %d)\n", bus,
		device, function);

	bool allocBars = AllocateBar();

	uint32 vendorID, deviceID;
	ReadConfig(NULL, bus, device, function, PCI_vendor_id, 2, &vendorID);
	ReadConfig(NULL, bus, device, function, PCI_device_id, 2, &deviceID);
	dprintf("  vendorID: %#04" B_PRIx32 "\n", vendorID);
	dprintf("  deviceID: %#04" B_PRIx32 "\n", deviceID);

	uint32 headerType;
	ReadConfig(NULL, bus, device, function, PCI_header_type, 1, &headerType);
	headerType = headerType % 0x80;

	dprintf("  headerType: ");
	switch (headerType) {
		case PCI_header_type_generic:
			dprintf("generic");
			break;
		case PCI_header_type_PCI_to_PCI_bridge:
			dprintf("bridge");
			break;
		case PCI_header_type_cardbus:
			dprintf("cardbus");
			break;
		default:
			dprintf("?(%u)", headerType);
	}
	dprintf("\n");

	if (headerType == PCI_header_type_PCI_to_PCI_bridge) {
		uint32 primaryBus, secondaryBus, subordinateBus;
		ReadConfig(NULL, bus, device, function, PCI_primary_bus, 1, &primaryBus);
		ReadConfig(NULL, bus, device, function, PCI_secondary_bus, 1, &secondaryBus);
		ReadConfig(NULL, bus, device, function, PCI_subordinate_bus, 1, &subordinateBus);
		dprintf("  primaryBus: %u\n", primaryBus);
		dprintf("  secondaryBus: %u\n", secondaryBus);
		dprintf("  subordinateBus: %u\n", subordinateBus);
	}

	uint32 oldValLo = 0, oldValHi = 0, sizeLo = 0, sizeHi = 0;
	uint64 val, size;
	for (int i = 0; i < ((headerType == PCI_header_type_PCI_to_PCI_bridge) ? 2 : 6); i++) {

		dprintf("  bar[%d]: ", i);

		ReadConfig(NULL, bus, device, function, PCI_base_registers + i*4, 4, &oldValLo);

		int regKind;
		if (oldValLo % 2 == 1) {
			regKind = kRegIo;
			dprintf("IOPORT");
		} else if (oldValLo / 2 % 4 == 0) {
			regKind = kRegMmio32;
			dprintf("MMIO32");
		} else if (oldValLo / 2 % 4 == 2) {
			regKind = kRegMmio64;
			dprintf("MMIO64");
		} else {
			dprintf("?(%d)", oldValLo / 2 % 4);
			dprintf("\n");
			continue;
		}

		ReadConfig(NULL, bus, device, function, PCI_base_registers + i*4, 4, &oldValLo);
		WriteConfig(NULL, bus, device, function, PCI_base_registers + i*4, 4, 0xffffffff);
		ReadConfig(NULL, bus, device, function, PCI_base_registers + i*4, 4, &sizeLo);
		WriteConfig(NULL, bus, device, function, PCI_base_registers + i*4, 4, oldValLo);

		val = oldValLo;
		size = sizeLo;
		if (regKind == kRegMmio64) {
			ReadConfig(NULL, bus, device, function, PCI_base_registers + (i + 1)*4, 4,
				&oldValHi);
			WriteConfig(NULL, bus, device, function, PCI_base_registers + (i + 1)*4, 4,
				0xffffffff);
			ReadConfig(NULL, bus, device, function, PCI_base_registers + (i + 1)*4, 4,
				&sizeHi);
			WriteConfig(NULL, bus, device, function, PCI_base_registers + (i + 1)*4, 4,
				oldValHi);
			val  += ((uint64)oldValHi) << 32;
			size += ((uint64)sizeHi  ) << 32;
		} else {
			if (sizeLo != 0)
				size += ((uint64)0xffffffff) << 32;
		}
		val &= ~(uint64)0xf;
		size = ~(size & ~(uint64)0xf) + 1;
/*
		dprintf(", oldValLo: 0x%" B_PRIx32 ", sizeLo: 0x%" B_PRIx32, oldValLo,
			sizeLo);
		if (regKind == regMmio64) {
			dprintf(", oldValHi: 0x%" B_PRIx32 ", sizeHi: 0x%" B_PRIx32,
				oldValHi, sizeHi);
		}
*/
		dprintf(", adr: 0x%" B_PRIx64 ", size: 0x%" B_PRIx64, val, size);

		if (allocBars && /*val == 0 &&*/ size != 0) {
			if (regKind == kRegMmio64) {
				val = AllocRegister(regKind, size);
				WriteConfig(NULL, bus, device, function,
					PCI_base_registers + (i + 0)*4, 4, (uint32)val);
				WriteConfig(NULL, bus, device, function,
					PCI_base_registers + (i + 1)*4, 4,
					(uint32)(val >> 32));
				dprintf(" -> 0x%" B_PRIx64, val);
			} else {
				val = AllocRegister(regKind, size);
				WriteConfig(NULL, bus, device, function,
					PCI_base_registers + i*4, 4, (uint32)val);
				dprintf(" -> 0x%" B_PRIx64, val);
			}
		}

		dprintf("\n");

		if (regKind == kRegMmio64)
			i++;
	}

	// ROM
	dprintf("  rom_bar: ");
	uint32 romBaseOfs = (headerType == PCI_header_type_PCI_to_PCI_bridge) ? PCI_bridge_rom_base : PCI_rom_base;
	ReadConfig(NULL, bus, device, function, romBaseOfs, 4, &oldValLo);
	WriteConfig(NULL, bus, device, function, romBaseOfs, 4, 0xfffffffe);
	ReadConfig(NULL, bus, device, function, romBaseOfs, 4, &sizeLo);
	WriteConfig(NULL, bus, device, function, romBaseOfs, 4, oldValLo);

	val = oldValLo & PCI_rom_address_mask;
	size = ~(sizeLo & ~(uint32)0xf) + 1;
	dprintf("adr: 0x%" B_PRIx64 ", size: 0x%" B_PRIx64, val, size);
	if (allocBars && /*val == 0 &&*/ size != 0) {
		val = AllocRegister(kRegMmio32, size);
		WriteConfig(NULL, bus, device, function,
			PCI_rom_base, 4, (uint32)val);
		dprintf(" -> 0x%" B_PRIx64, val);
	}
	dprintf("\n");

	uint32 intPin;
	ReadConfig(NULL, bus, device, function,
		PCI_interrupt_pin, 1, &intPin);

	InterruptMap* intMap = LookupInterruptMap(EncodePciAddress(bus, device, function), intPin);
	if (intMap == NULL) {
		dprintf("no interrupt mapping for childAdr: (%d:%d:%d), childIrq: %d)\n", bus,
			device, function, intPin);
	} else {
		WriteConfig(NULL, bus, device, function, PCI_interrupt_line,
			1, intMap->parentIrq);
	}

	InitDeviceMSI(bus, device, function);

	uint32 intLine;
	ReadConfig(NULL, bus, device, function, PCI_interrupt_line, 1, &intLine);
	dprintf("  intLine: %u\n", intLine);
	dprintf("  intPin: ");
	switch (intPin) {
		case 0:
			dprintf("-");
			break;
		case 1:
			dprintf("INTA#");
			break;
		case 2:
			dprintf("INTB#");
			break;
		case 3:
			dprintf("INTC#");
			break;
		case 4:
			dprintf("INTD#");
			break;
		default:
			dprintf("?(%u)", intPin);
			break;
	}
	dprintf("\n");
}


static status_t
read_pci_config(void *cookie, uint8 bus, uint8 device, uint8 function,
	uint16 offset, uint8 size, uint32 *value)
{
	if (gArchPCI == NULL)
		return B_ERROR;

	// PciConfigAdr may use sliding window
	// TODO: SMP
	InterruptsLocker locker;

	return gArchPCI->ReadConfig(cookie, bus, device, function, offset, size, value);
}


static status_t
write_pci_config(void *cookie, uint8 bus, uint8 device, uint8 function,
	uint16 offset, uint8 size, uint32 value)
{
	if (gArchPCI == NULL)
		return B_ERROR;

	// PciConfigAdr may use sliding window
	// TODO: SMP
	InterruptsLocker locker;
/*
	dprintf("write_pci_config(%d:%d:%d, 0x%04x, %d, 0x%08x)\n", bus, device,
		function, offset, size, value);
*/
	return gArchPCI->WriteConfig(cookie, bus, device, function, offset, size, value);
}


static status_t
get_max_bus_devices(void *cookie, int32 *count)
{
	*count = 32;
	return B_OK;
}


status_t
read_pci_irq(void *cookie, uint8 bus, uint8 device, uint8 function, uint8 pin,
	uint8 *irq)
{
	return B_NOT_SUPPORTED;
}


status_t
write_pci_irq(void *cookie, uint8 bus, uint8 device, uint8 function, uint8 pin,
	uint8 irq)
{
	return B_NOT_SUPPORTED;
}


pci_controller pci_controller_riscv64 =
{
	read_pci_config,
	write_pci_config,
	get_max_bus_devices,
	read_pci_irq,
	write_pci_irq,
};


//#pragma mark -


status_t
pci_controller_init()
{
	dprintf("pci_controller_init()\n");

	dprintf("sizeof(PciDbi): %#" B_PRIxSIZE "\n", sizeof(PciDbiRegs));

	if (gPCIRootNode == NULL)
		return B_OK;

	DeviceNodePutter<&gDeviceManager> parent(
		gDeviceManager->get_parent_node(gPCIRootNode));
	fdt_device_module_info *parentModule;

	const char* compatible;
	if (gDeviceManager->get_attr_string(parent.Get(), "fdt/compatible", &compatible,
		false) < B_OK)
		return B_ERROR;

	dprintf("hostCtrlType: ");
	if (strcmp(compatible, "pci-host-ecam-generic") == 0) {
		static char buffer[sizeof(PCIEcam)];
		gArchPCI = new(buffer) PCIEcam();
		dprintf("ecam\n");
	} else if (strcmp(compatible, "sifive,fu740-pcie") == 0) {
		static char buffer[sizeof(PCIFU740)];
		gArchPCI = new(buffer) PCIFU740();
		dprintf("sifive\n");
	} else {
		dprintf("unknown\n");
		return B_ERROR;
	}

	if (gArchPCI == NULL)
		return B_ERROR;

	status_t result = B_ERROR;

	// Init our detected PCI bus
	result = gArchPCI->Init(parentModule);
	if (result != B_OK)
		return result;

	return pci_controller_add(&pci_controller_riscv64, NULL);
}


phys_addr_t
pci_ram_address(phys_addr_t childAdr)
{
	// dprintf("pci_ram_address(0x%" B_PRIxPHYSADDR "): ", childAdr);
	phys_addr_t parentAdr = 0;
	for (int kind = kRegIo; kind <= kRegMmio64; kind++) {
		auto range = gArchPCI->GetRegisterRange(kind);
		if (range == NULL) {
			dprintf("%s: Invalid register range %d!\n", __func__, kind);
			return 0;
		}
		if (childAdr >= range->childBase
			&& childAdr < range->childBase + range->size) {
			parentAdr = childAdr - range->childBase;
			if (kind != kRegIo)
				parentAdr += range->parentBase;
			// dprintf("0x%" B_PRIxPHYSADDR "\n", parentAdr);
			return parentAdr;
		}
	}
	// dprintf("?\n");
	return 0;
}
