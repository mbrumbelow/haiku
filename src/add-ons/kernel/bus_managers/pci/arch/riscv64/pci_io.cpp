/*
 * Copyright 2009-2020, Haiku Inc., All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "pci_io.h"
#include "pci_private.h"

#include <AutoDeleterOS.h>


extern addr_t gPCIeIoBase;
extern size_t gPCIeIoSize;


status_t
pci_io_init()
{
	dprintf("pci_io_init()\n");
	return B_OK;
}


uint8
pci_read_io_8(int mapped_io_addr)
{
	dprintf("pci_read_io_8(%d)\n", mapped_io_addr);
	volatile uint8* ptr = (uint8*)(gPCIeIoBase + mapped_io_addr);
	return *ptr;
}


void
pci_write_io_8(int mapped_io_addr, uint8 value)
{
	dprintf("pci_write_io_8(%d)\n", mapped_io_addr);
	volatile uint8* ptr = (uint8*)(gPCIeIoBase + mapped_io_addr);
	*ptr = value;
}


uint16
pci_read_io_16(int mapped_io_addr)
{
	dprintf("pci_read_io_16(%d)\n", mapped_io_addr);
	volatile uint16* ptr = (uint16*)(gPCIeIoBase + mapped_io_addr);
	return *ptr;
}


void
pci_write_io_16(int mapped_io_addr, uint16 value)
{
	dprintf("pci_write_io_16(%d)\n", mapped_io_addr);
	volatile uint16* ptr = (uint16*)(gPCIeIoBase + mapped_io_addr);
	*ptr = value;
}


uint32
pci_read_io_32(int mapped_io_addr)
{
	dprintf("pci_read_io_32(%d)\n", mapped_io_addr);
	volatile uint32* ptr = (uint32*)(gPCIeIoBase + mapped_io_addr);
	return *ptr;
}


void
pci_write_io_32(int mapped_io_addr, uint32 value)
{
	dprintf("pci_write_io_32(%d)\n", mapped_io_addr);
	volatile uint32* ptr = (uint32*)(gPCIeIoBase + mapped_io_addr);
	*ptr = value;
}
