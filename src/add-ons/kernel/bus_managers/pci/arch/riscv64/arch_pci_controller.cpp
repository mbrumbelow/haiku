/*
 * Copyright 2021, Haiku Inc., All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *	x512 <danger_mail@list.ru>
 *	Alexander von Gluck IV <kallisti5@unixzen.com>
 */


#include "arch_pci_controller.h"

#include "fu740/pci_fu740.h"
#include "ecam/pci_ecam.h"


extern PCI* gPCI;
ArchPCIController* gArchPCI = NULL;


static status_t read_pci_config(void *cookie, uint8 bus, uint8 device, uint8 function,
	uint16 offset, uint8 size, uint32 *value);
static status_t write_pci_config(void *cookie, uint8 bus, uint8 device, uint8 function,
	uint16 offset, uint8 size, uint32 value);


static PciBarKind
GetPciBarKind(uint32 val)
{
	if (val % 2 == 1)
		return kRegIo;
	if (val / 2 % 4 == 0)
		return kRegMmio32;
	if (val / 2 % 4 == 1)
		return kRegMmio1MB;
	if (val / 2 % 4 == 2)
		return kRegMmio64;
	return kRegUnknown;
}


static void
GetBarValMask(uint32& val, uint32& mask, uint8 bus, uint8 device, uint8 function, uint16 offset)
{
	val = 0;
	mask = 0;
	read_pci_config (NULL, bus, device, function, offset, 4, &val);
	write_pci_config(NULL, bus, device, function, offset, 4, 0xffffffff);
	read_pci_config (NULL, bus, device, function, offset, 4, &mask);
	write_pci_config(NULL, bus, device, function, offset, 4, val);
}


static void
GetBarKindValSize(PciBarKind& barKind, uint64& val, uint64& size,
	uint8 bus, uint8 device, uint8 function, uint16 offset)
{
	uint32 oldValLo = 0, oldValHi = 0, sizeLo = 0, sizeHi = 0;
	GetBarValMask(oldValLo, sizeLo, bus, device, function, offset);
	barKind = GetPciBarKind(oldValLo);
	val = oldValLo;
	size = sizeLo;
	if (barKind == kRegMmio64) {
		GetBarValMask(oldValHi, sizeHi, bus, device, function, offset + 4);
		val  += ((uint64)oldValHi) << 32;
		size += ((uint64)sizeHi  ) << 32;
	} else {
		if (sizeLo != 0)
			size += ((uint64)0xffffffff) << 32;
	}
	if (barKind == kRegIo)
		val &= ~(uint64)0x3;
	else
		val &= ~(uint64)0xf;
	size = ~(size & ~(uint64)0xf) + 1;
}


static void
SetBarVal(uint8 bus, uint8 device, uint8 function, uint16 offset, PciBarKind barKind, uint64 val)
{
	write_pci_config(NULL, bus, device, function, offset, 4, (uint32)val);
	if (barKind == kRegMmio64)
		write_pci_config(NULL, bus, device, function, offset + 4, 4, (uint32)(val >> 32));
}


ArchPCIController::ArchPCIController()
	:
	fMsiPhysAddr(0),
	fMsiStartIrq(0),
	fMsiData(0),
	fHostCtrlType(0),
	fConfigPhysBase(0),
	fConfigBase(0),
	fConfigSize(0),
	fIoBase(0),
	fInterruptMapLen(0)
{
	fAllocatedMsiIrqs[0] = 0;
}


ArchPCIController::~ArchPCIController()
{
}


uint32_t
ArchPCIController::EncodePciAddress(uint8 bus, uint8 device, uint8 function)
{
	return bus % (1 << 8) * (1 << 16)
		+ device % (1 << 5) * (1 << 11)
		+ function % (1 << 3) * (1 << 8);
}


void
ArchPCIController::DecodePciAddress(uint32_t adr, uint8& bus, uint8& device, uint8& function)
{
	bus = adr / (1 << 16) % (1 << 8);
	device = adr / (1 << 11) % (1 << 5);
	function = adr / (1 << 8) % (1 << 3);
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


static uint32 ReadReg8(addr_t adr)
{
	uint32 ofs = adr % 4;
	adr = adr / 4 * 4;
	union {
		uint32 in;
		uint8 out[4];
	} val{.in = *(vuint32*)adr};
	return val.out[ofs];
}

static uint32 ReadReg16(addr_t adr)
{
	uint32 ofs = adr / 2 % 2;
	adr = adr / 4 * 4;
	union {
		uint32 in;
		uint16 out[2];
	} val{.in = *(vuint32*)adr};
	return val.out[ofs];
}

static void WriteReg8(addr_t adr, uint32 value)
{
	uint32 ofs = adr % 4;
	adr = adr / 4 * 4;
	union {
		uint32 in;
		uint8 out[4];
	} val{.in = *(vuint32*)adr};
	val.out[ofs] = (uint8)value;
	*(vuint32*)adr = val.in;
}

static void WriteReg16(addr_t adr, uint32 value)
{
	uint32 ofs = adr / 2 % 2;
	adr = adr / 4 * 4;
	union {
		uint32 in;
		uint16 out[2];
	} val{.in = *(vuint32*)adr};
	val.out[ofs] = (uint16)value;
	*(vuint32*)adr = val.in;
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
			*value = ReadReg8(address);
			break;
		case 2:
			*value = ReadReg16(address);
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
			WriteReg8(address, value);
			break;
		case 2:
			WriteReg16(address, value);
			break;
		case 4:
			*(uint32*)address = value;
			break;
		default:
			return B_ERROR;
	}

	return B_OK;
}



bool
ArchPCIController::AllocBar(uint8 bus, uint8 device, uint8 function, uint16 offset)
{
	bool allocBars = AllocateBar();

	PciBarKind regKind;
	uint64 val, size;
	GetBarKindValSize(regKind, val, size, bus, device, function, offset);
	switch (regKind) {
		case kRegIo:     dprintf("IOPORT"); break;
		case kRegMmio32: dprintf("MMIO32"); break;
		case kRegMmio64: dprintf("MMIO64"); break;
		default:
			dprintf("?(%#x)", (unsigned)(val%16));
			dprintf("\n");
			return false;
	}

	dprintf(", adr: 0x%" B_PRIx64 ", size: 0x%" B_PRIx64, val, size);

	if (allocBars && size != 0) {
		val = AllocRegister(regKind, size);
		SetBarVal(bus, device, function, offset, regKind, val);
		dprintf(" -> 0x%" B_PRIx64, val);
	}

	dprintf("\n");

	return regKind == kRegMmio64;
}


void
ArchPCIController::AllocRegsForDevice(uint8 bus, uint8 device, uint8 function)
{
	dprintf("AllocRegsForDevice(bus: %d, device: %d, function: %d)\n", bus, device, function);

	uint32 vendorID = 0, deviceID = 0;
	read_pci_config(NULL, bus, device, function, PCI_vendor_id, 2, &vendorID);
	read_pci_config(NULL, bus, device, function, PCI_device_id, 2, &deviceID);
	dprintf("  vendorID: %#04" B_PRIx32 "\n", vendorID);
	dprintf("  deviceID: %#04" B_PRIx32 "\n", deviceID);

	uint32 headerType = 0;
	read_pci_config(NULL, bus, device, function, PCI_header_type, 1, &headerType);
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
		uint32 primaryBus = 0, secondaryBus = 0, subordinateBus = 0;
		read_pci_config(NULL, bus, device, function, PCI_primary_bus, 1, &primaryBus);
		read_pci_config(NULL, bus, device, function, PCI_secondary_bus, 1, &secondaryBus);
		read_pci_config(NULL, bus, device, function, PCI_subordinate_bus, 1, &subordinateBus);
		dprintf("  primaryBus: %u\n", primaryBus);
		dprintf("  secondaryBus: %u\n", secondaryBus);
		dprintf("  subordinateBus: %u\n", subordinateBus);
	}

	for (int i = 0; i < ((headerType == PCI_header_type_PCI_to_PCI_bridge) ? 2 : 6); i++) {
		dprintf("  bar[%d]: ", i);
		if (AllocBar(bus, device, function, PCI_base_registers + i*4))
			i++;
	}
	// ROM
	dprintf("  romBar: ");
	uint32 romBaseOfs = (headerType == PCI_header_type_PCI_to_PCI_bridge)
		? PCI_bridge_rom_base : PCI_rom_base;
	AllocBar(bus, device, function, romBaseOfs);

	uint32 intPin = 0;
	read_pci_config(NULL, bus, device, function, PCI_interrupt_pin, 1, &intPin);

	InterruptMap* intMap = LookupInterruptMap(EncodePciAddress(bus, device, function), intPin);
	if (intMap == NULL)
		dprintf("no interrupt mapping for childAdr: (%d:%d:%d), childIrq: %d)\n",
			bus, device, function, intPin);
	else
		write_pci_config(NULL, bus, device, function, PCI_interrupt_line, 1, intMap->parentIrq);

	uint32 intLine = 0;
	read_pci_config(NULL, bus, device, function, PCI_interrupt_line, 1, &intLine);
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

	result = gArchPCI->Init(gPCIRootNode);
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
