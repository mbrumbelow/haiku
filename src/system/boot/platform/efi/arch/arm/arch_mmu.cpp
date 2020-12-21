/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Based on code written by Travis Geiselbrecht for NewOS.
 *
 * Distributed under the terms of the MIT License.
 */


#include "arch_mmu.h"

#include <algorithm>
#include <boot/platform.h>
#include <boot/stdio.h>
#include <boot/kernel_args.h>
#include <boot/stage2.h>
#include <arch/cpu.h>
#include <arch_kernel.h>
#include <arm_mmu.h>
#include <kernel.h>

#include <OS.h>

#include <string.h>

#include "fdt_support.h"

extern "C" {
#include <fdt.h>
#include <libfdt.h>
#include <libfdt_env.h>
};

#include "mmu.h"


#define TRACE_MMU
#ifdef TRACE_MMU
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


/*
 * Set translation table base
 */
void
mmu_set_TTBR(uint32 ttb)
{
	ttb &= 0xffffc000;
	asm volatile("MCR p15, 0, %[adr], c2, c0, 0"::[adr] "r" (ttb));
}


/*
 * Flush the TLB
 */
void
mmu_flush_TLB()
{
	uint32 value = 0;
	asm volatile("MCR p15, 0, %[c8format], c8, c7, 0"::[c8format] "r" (value));
}


/*
 * Read MMU Control Register
 */
uint32
mmu_read_C1()
{
	uint32 controlReg = 0;
	asm volatile("MRC p15, 0, %[c1out], c1, c0, 0":[c1out] "=r" (controlReg));
	return controlReg;
}


/*
 * Write MMU Control Register
 */
void
mmu_write_C1(uint32 value)
{
	asm volatile("MCR p15, 0, %[c1in], c1, c0, 0"::[c1in] "r" (value));
}


/*
 * Dump current MMU Control Register state
 * For debugging, can be added to loader temporarly post-serial-init
 */
void
mmu_dump_C1()
{
	uint32 cpValue = mmu_read_C1();

	dprintf("MMU CP15:c1 State:\n");

	if ((cpValue & (1 << 0)) != 0)
		dprintf(" - MMU Enabled\n");
	else
		dprintf(" - MMU Disabled\n");

	if ((cpValue & (1 << 2)) != 0)
		dprintf(" - Data Cache Enabled\n");
	else
		dprintf(" - Data Cache Disabled\n");

	if ((cpValue & (1 << 3)) != 0)
		dprintf(" - Write Buffer Enabled\n");
	else
		dprintf(" - Write Buffer Disabled\n");

	if ((cpValue & (1 << 12)) != 0)
		dprintf(" - Instruction Cache Enabled\n");
	else
		dprintf(" - Instruction Cache Disabled\n");

	if ((cpValue & (1 << 13)) != 0)
		dprintf(" - Vector Table @ 0xFFFF0000\n");
	else
		dprintf(" - Vector Table @ 0x00000000\n");
}


void
mmu_write_DACR(uint32 value)
{
	asm volatile("MCR p15, 0, %[c1in], c3, c0, 0"::[c1in] "r" (value));
}


//TODO:move this to generic/ ?
static status_t
fdt_map_memory_ranges(void* fdt, const char* path, bool physical = false)
{
	int node;
	const void *prop;
	int len;
	uint64 total;

	if (fdt == NULL) {
		dprintf("%s: fdt missing! Unable to map %s.\n", __func__, path);
		return B_ERROR;
	}

	dprintf("checking FDT for %s...\n", path);
	node = fdt_path_offset(fdt, path);

	total = 0;

	int32 regAddressCells = 1;
	int32 regSizeCells = 1;
	fdt_get_cell_count(fdt, node, regAddressCells, regSizeCells);

	prop = fdt_getprop(fdt, node, "reg", &len);
	if (prop == NULL) {
		dprintf("Unable to locate %s in FDT!\n", path);
		return B_ERROR;
	}

	const uint32 *p = (const uint32 *)prop;
	for (int32 i = 0; len; i++) {
		uint64 base;
		uint64 size;
		if (regAddressCells == 2)
			base = fdt64_to_cpu(*(uint64_t *)p);
		else
			base = fdt32_to_cpu(*(uint32_t *)p);
		p += regAddressCells;
		if (regSizeCells == 2)
			size = fdt64_to_cpu(*(uint64_t *)p);
		else
			size = fdt32_to_cpu(*(uint32_t *)p);
		p += regSizeCells;
		len -= sizeof(uint32) * (regAddressCells + regSizeCells);

		if (size <= 0) {
			dprintf("%ld: empty region\n", i);
			continue;
		}
		dprintf("%" B_PRIu32 ": base = %" B_PRIu64 ","
			"size = %" B_PRIu64 "\n", i, base, size);

		total += size;

		if (physical) {
			if (mmu_map_physical_memory(base, size,
					kDefaultPageFlags) < 0) {
				dprintf("cannot map physical memory range "
					"(num ranges = %" B_PRIu32 ")!\n",
					gKernelArgs.num_physical_memory_ranges);
				return B_ERROR;
			}
		} else {
			// TODO: virtual valid here?
		}
	}

	dprintf("total '%s' physical memory = %" B_PRId64 "MB\n", path,
		total / (1024 * 1024));

	return B_OK;
}


static void
fdt_map_peripheral(void* fdt)
{
	// map peripheral devices (such as uart) from fdt

	#warning Map peripherals from the fdt we want to use in the bootloader!
	// this assumes /pl011@9000000 which is the qemu virt uart
	fdt_map_memory_ranges(fdt, "/pl011@9000000");
	// this assumes /axi which is broadcom!
	fdt_map_memory_ranges(fdt, "/axi");
}


void
arch_mmu_init()
{
	TRACE(("arch_mmu_init\n"));

	mmu_dump_C1();
}


uint64_t
arch_mmu_generate_post_efi_page_tables(size_t memory_map_size,
	efi_memory_descriptor *memory_map, size_t descriptor_size,
	uint32_t descriptor_version)
{
	// Generate page tables

	// Page Global Directory
	uint32_t *pgd;

	// Page Table Entry
	uint32_t *pte;

	//uint64_t *pdpt;
	//uint64_t *pageDir;
	//uint64_t *pageTable;

	// Allocate the top level page global directory.
	pgd = NULL;
	if (platform_allocate_region((void**)&pgd, B_PAGE_SIZE, 0, false) != B_OK)
		panic("Failed to allocate page global directory!");
	gKernelArgs.arch_args.phys_pgdir = (uint32_t)(addr_t)pgd;
	memset(pgd, 0, B_PAGE_SIZE);

	// Not really needed since 32-bit?
	//platform_bootloader_address_to_kernel_address(pgd,
	//	&gKernelArgs.arch_args.vir_pgdir);

	// Store the virtual memory usage information.
	gKernelArgs.virtual_allocated_range[0].start = KERNEL_LOAD_BASE;
	gKernelArgs.virtual_allocated_range[0].size
		= get_current_virtual_address() - KERNEL_LOAD_BASE;
	gKernelArgs.num_virtual_allocated_ranges = 1;
	gKernelArgs.arch_args.virtual_end = ROUNDUP(KERNEL_LOAD_BASE
		+ gKernelArgs.virtual_allocated_range[0].size, 0x200000);

	// Find the highest physical memory address. We map all physical memory
	// into the kernel address space, so we want to make sure we map everything
	// we have available.
	uint32 maxAddress = 0;
	for (size_t i = 0; i < memory_map_size / descriptor_size; ++i) {
		efi_memory_descriptor *entry
			= (efi_memory_descriptor *)((addr_t)memory_map + i * descriptor_size);
		maxAddress = std::max(maxAddress,
				      (uint32)(entry->PhysicalStart + entry->NumberOfPages * 4096));
	}

	// Want to map at least 4GB, there may be stuff other than usable RAM that
	// could be in the first 4GB of physical address space.
	maxAddress = std::max(maxAddress, (uint32)0x100000000);
	maxAddress = ROUNDUP(maxAddress, 0x40000000);

	// Currently only use 1 PDPT (512GB). This will need to change if someone
	// wants to use Haiku on a box with more than 512GB of RAM but that's
	// probably not going to happen any time soon.
	if (maxAddress / 0x40000000 > 512)
		panic("Can't currently support more than 512GB of RAM!");

	// Create page tables for the physical map area. Also map this PDPT
	// temporarily at the bottom of the address space so that we are identity
	// mapped.

	pte = (uint32*)mmu_allocate_page();
	memset(pte, 0, B_PAGE_SIZE);
	pgd[510] = (addr_t)pte | kTableMappingFlags;
	pgd[0] = (addr_t)pte | kTableMappingFlags;

	#if 0
	for (uint64 i = 0; i < maxAddress; i += 0x40000000) {
		pageDir = (uint64*)mmu_allocate_page();
		memset(pageDir, 0, B_PAGE_SIZE);
		pte[i / 0x40000000] = (addr_t)pageDir | kTableMappingFlags;

		for (uint64 j = 0; j < 0x40000000; j += 0x200000) {
			pageDir[j / 0x200000] = (i + j) | kLargePageMappingFlags;
		}
	}
	// Allocate tables for the kernel mappings.

	pdpt = (uint64*)mmu_allocate_page();
	memset(pdpt, 0, B_PAGE_SIZE);
	pml4[511] = (addr_t)pdpt | kTableMappingFlags;

	pageDir = (uint64*)mmu_allocate_page();
	memset(pageDir, 0, B_PAGE_SIZE);
	pdpt[510] = (addr_t)pageDir | kTableMappingFlags;

	// We can now allocate page tables and duplicate the mappings across from
	// the 32-bit address space to them.
	pageTable = NULL; // shush, compiler.
	for (uint32 i = 0; i < gKernelArgs.virtual_allocated_range[0].size
			/ B_PAGE_SIZE; i++) {
		if ((i % 512) == 0) {
			pageTable = (uint64*)mmu_allocate_page();
			memset(pageTable, 0, B_PAGE_SIZE);
			pageDir[i / 512] = (addr_t)pageTable | kTableMappingFlags;
		}

		// Get the physical address to map.
		void *phys;
		if (platform_kernel_address_to_bootloader_address(
			KERNEL_LOAD_BASE + (i * B_PAGE_SIZE), &phys) != B_OK) {
			continue;
		}

		pageTable[i % 512] = (addr_t)phys | kPageMappingFlags;
	}
	#endif

	return (uint64)pgd;
}


// Called after EFI boot services exit.
// Currently assumes that the memory map is sane... Sorted and no overlapping
// regions.
void
arch_mmu_post_efi_setup(size_t memory_map_size,
	efi_memory_descriptor *memory_map, size_t descriptor_size,
	uint32_t descriptor_version)
{
	// Add physical memory to the kernel args and update virtual addresses for
	// EFI regions.
	addr_t addr = (addr_t)memory_map;
	gKernelArgs.num_physical_memory_ranges = 0;

	// First scan: Add all usable ranges
	for (size_t i = 0; i < memory_map_size / descriptor_size; ++i) {
		efi_memory_descriptor *entry
			= (efi_memory_descriptor *)(addr + i * descriptor_size);
		switch (entry->Type) {
		case EfiLoaderCode:
		case EfiLoaderData:
		case EfiBootServicesCode:
		case EfiBootServicesData:
		case EfiConventionalMemory: {
			// Usable memory.
			// Ignore memory below 1MB and above 512GB.
			uint64_t base = entry->PhysicalStart;
			uint64_t end = entry->PhysicalStart + entry->NumberOfPages * 4096;
			uint64_t originalSize = end - base;
			if (base < 0x100000)
				base = 0x100000;
			if (end > (512ull * 1024 * 1024 * 1024))
				end = 512ull * 1024 * 1024 * 1024;

			gKernelArgs.ignored_physical_memory
				+= originalSize - (max_c(end, base) - base);

			if (base >= end)
				break;
			uint64_t size = end - base;

			insert_physical_memory_range(base, size);
			// LoaderData memory is bootloader allocated memory, possibly
			// containing the kernel or loaded drivers.
			if (entry->Type == EfiLoaderData)
				insert_physical_allocated_range(base, size);
			break;
		}
		case EfiACPIReclaimMemory:
			// ACPI reclaim -- physical memory we could actually use later
			break;
		case EfiRuntimeServicesCode:
		case EfiRuntimeServicesData:
			entry->VirtualStart = entry->PhysicalStart;
			break;
		}
	}

	uint64_t initialPhysicalMemory = total_physical_memory();

	// Second scan: Remove everything reserved that may overlap
	for (size_t i = 0; i < memory_map_size / descriptor_size; ++i) {
		efi_memory_descriptor *entry
			= (efi_memory_descriptor *)(addr + i * descriptor_size);
		switch (entry->Type) {
		case EfiLoaderCode:
		case EfiLoaderData:
		case EfiBootServicesCode:
		case EfiBootServicesData:
		case EfiConventionalMemory:
			break;
		default:
			uint64_t base = entry->PhysicalStart;
			uint64_t end = entry->PhysicalStart + entry->NumberOfPages * 4096;
			remove_physical_memory_range(base, end - base);
		}
	}

	gKernelArgs.ignored_physical_memory
		+= initialPhysicalMemory - total_physical_memory();

	// Sort the address ranges.
	sort_address_ranges(gKernelArgs.physical_memory_range,
		gKernelArgs.num_physical_memory_ranges);
	sort_address_ranges(gKernelArgs.physical_allocated_range,
		gKernelArgs.num_physical_allocated_ranges);
	sort_address_ranges(gKernelArgs.virtual_allocated_range,
		gKernelArgs.num_virtual_allocated_ranges);

	// Switch EFI to virtual mode, using the kernel pmap.
	// Something involving ConvertPointer might need to be done after this?
	// http://wiki.phoenix.com/wiki/index.php/EFI_RUNTIME_SERVICES
	kRuntimeServices->SetVirtualAddressMap(memory_map_size, descriptor_size,
		descriptor_version, memory_map);

	// Make sure supervisor threads can fault on read only pages on ARM?
}
