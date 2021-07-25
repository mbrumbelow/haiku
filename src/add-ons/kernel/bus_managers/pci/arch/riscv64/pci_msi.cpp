/*
 * Copyright 2021, Haiku Inc., All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "pci_msi.h"
#include "pci_controller_private.h"
#include <int.h>

#include <kernel/debug.h>


long gStartMsiIrq = 0;
static uint32 sAllocatedMsiIrqs[1] = {0};
static uint64 sMsiData = 0;
phys_addr_t gMsiPhysAddr = 0;


static inline volatile PciDbiRegs*
GetDbuRegs()
{
	return (PciDbiRegs*)gPCIeDbiBase;
}


int32
AllocMsiInterrupt()
{
	for (int i = 0; i < 32; i++) {
		if (((1 << i) & sAllocatedMsiIrqs[0]) == 0) {
			sAllocatedMsiIrqs[0] |= (1 << i);
			GetDbuRegs()->msiIntr[0].mask &= ~(1 << i);
			return i;
		}
	}
	return B_ERROR;
}


void
FreeMsiInterrupt(int32 irq)
{
	if (irq >= 0 && irq < 32 && ((1 << irq) & sAllocatedMsiIrqs[0]) != 0) {
		GetDbuRegs()->msiIntr[0].mask |= (1 << (uint32)irq);
		sAllocatedMsiIrqs[0] &= ~(1 << (uint32)irq);
	}
}


static int32
MsiInterruptHandler(void* arg)
{
//	dprintf("MsiInterruptHandler()\n");
	uint32 status = GetDbuRegs()->msiIntr[0].status;
	for (int i = 0; i < 32; i++) {
		if (((1 << i) & status) != 0) {
//			dprintf("MSI IRQ: %d (%ld)\n", i, gStartMsiIrq + i);
			int_io_interrupt_handler(gStartMsiIrq + i, false);
			GetDbuRegs()->msiIntr[0].status = (1 << i);
		}
	}
	return B_HANDLED_INTERRUPT;
}


void
InitPciMsi(int32 msiIrq)
{
	dprintf("InitPciMsi()\n");
	dprintf("  msiIrq: %" B_PRId32 "\n", msiIrq);

	physical_entry pe;
	ASSERT_ALWAYS(get_memory_map(&sMsiData, sizeof(sMsiData), &pe, 1) >= B_OK);
	gMsiPhysAddr = pe.address;
	dprintf("  gMsiPhysAddr: %#" B_PRIxADDR "\n", gMsiPhysAddr);
	GetDbuRegs()->msiAddrLo = (uint32)gMsiPhysAddr;
	GetDbuRegs()->msiAddrHi = (uint32)(gMsiPhysAddr >> 32);

	GetDbuRegs()->msiIntr[0].enable = 0xffffffff;
	GetDbuRegs()->msiIntr[0].mask = 0xffffffff;

	ASSERT_ALWAYS(install_io_interrupt_handler(msiIrq, MsiInterruptHandler, NULL, 0) >= B_OK);
	ASSERT_ALWAYS(allocate_io_interrupt_vectors(32, &gStartMsiIrq, INTERRUPT_TYPE_IRQ) >= B_OK);
	dprintf("  gStartMsiIrq: %ld\n", gStartMsiIrq);
}
