/*
 * Copyright 2019-2021 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
 */

#include <algorithm>

#include <kernel.h>
#include <arch_kernel.h>
#include <boot/platform.h>
#include <boot/stage2.h>

#include <efi/types.h>
#include <efi/boot-services.h>

//#include "mmu.h"
#include "efi_platform.h"


// RISC-V Privileged Architectures V1.10, section 4.1.12
//
// RV64 Supervisor Address Transaltion and Protection (satp)
// Physical Page Number of the root page table
// (supervisor physical address divided by 4KiB)
#define RV64_SATP_PPN_SHIFT	0x0
#define RV64_SATP_PPN_MASK	0x00000FFFFFFFFFFF

// Address Space Identifier
// (facilitates address-transation fences on a per-address-space basis)
#define RV64_SATP_ASID_SHIFT	0x2C
#define RV64_SATP_PPN_MASK	0x0FFFF00000000000

// Mode (selects the current address-translation scheme)
#define RV64_SATP_MODE_SHIFT	0x3C
#define RV64_SATP_MODE_MASK	0xF000000000000000
#define RV64_SATP_MODE_NONE	0x0
#define RV64_SATP_MODE_SV39	0x8
#define RV64_SATP_MODE_SV48	0x9

// Page Flags
#define RISCV_PAGE_VALID		(1 << 0)
#define RISCV_PAGE_R			(1 << 1)
#define RISCV_PAGE_W			(1 << 2)
#define RISCV_PAGE_VALID_RW		(3 << 0)
#define RISCV_PAGE_EX			(1 << 3)
#define RISCV_PAGE_USER			(1 << 4)
#define RISCV_PAGE_GLOBAL		(1 << 5)
#define RISCV_PAGE_ACCESSED		(1 << 6)
#define RISCV_PAGE_DIRTY		(1 << 7)


static const uint64 kDefaultPageFlags =	RISCV_PAGE_VALID_RW;
static const uint64 kTableMappingFlags = RISCV_PAGE_VALID_RW | RISCV_PAGE_USER;
static const uint64 kPageMappingFlags = RISCV_PAGE_VALID_RW
	| RISCV_PAGE_USER | RISCV_PAGE_GLOBAL;


void
arch_mmu_init()
{
	// Stub
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

	// Important.  Make sure supervisor threads can fault on read only pages...
	//asm("mov %%rax, %%cr0" : : "a" ((1 << 31) | (1 << 16) | (1 << 5) | 1));
}

uint64_t
arch_mmu_generate_post_efi_page_tables(size_t memory_map_size,
	efi_memory_descriptor *memory_map, size_t descriptor_size,
	uint32_t descriptor_version)
{
	uint64_t *sv39;
	//uint64_t *pdpt;
	//uint64_t *pageDir;
	//uint64_t *pageTable;

	// Allocate the top level sv39 PTE
	sv39 = NULL;
	if (platform_allocate_region((void**)&sv39, B_PAGE_SIZE, 0, false) != B_OK)
		panic("Failed to allocate SV39.");
	gKernelArgs.arch_args.phys_pgdir = (uint64_t)(addr_t)sv39;
	memset(sv39, 0, B_PAGE_SIZE);
	platform_bootloader_address_to_kernel_address(sv39,
		&gKernelArgs.arch_args.vir_pgdir);

	// Store the virtual memory usage information.
	gKernelArgs.virtual_allocated_range[0].start = KERNEL_LOAD_BASE_64_BIT;
	gKernelArgs.virtual_allocated_range[0].size
		= get_current_virtual_address() - KERNEL_LOAD_BASE_64_BIT;
	gKernelArgs.num_virtual_allocated_ranges = 1;
	gKernelArgs.arch_args.virtual_end = ROUNDUP(KERNEL_LOAD_BASE_64_BIT
		+ gKernelArgs.virtual_allocated_range[0].size, 0x200000);

	// Find the highest physical memory address. We map all physical memory
	// into the kernel address space, so we want to make sure we map everything
	// we have available.
	uint64 maxAddress = 0;
	for (size_t i = 0; i < memory_map_size / descriptor_size; ++i) {
		efi_memory_descriptor *entry
			= (efi_memory_descriptor *)((addr_t)memory_map + i * descriptor_size);
		maxAddress = std::max(maxAddress,
				      entry->PhysicalStart + entry->NumberOfPages * 4096);
	}
	// Want to map at least 4GB, there may be stuff other than usable RAM that
	// could be in the first 4GB of physical address space.
	maxAddress = std::max(maxAddress, (uint64)0x100000000ll);
	maxAddress = ROUNDUP(maxAddress, 0x40000000);

	// Currently only use 1 PDPT (512GB). This will need to change if someone
	// wants to use Haiku on a box with more than 512GB of RAM but that's
	// probably not going to happen any time soon.
	if (maxAddress / 0x40000000 > 512)
		panic("Can't currently support more than 512GB of RAM!");

	#if 0
	// Create page tables for the physical map area. Also map this PDPT
	// temporarily at the bottom of the address space so that we are identity
	// mapped.

	pdpt = (uint64*)mmu_allocate_page();
	memset(pdpt, 0, B_PAGE_SIZE);
	sv39[510] = (addr_t)pdpt | kTableMappingFlags;
	sv39[0] = (addr_t)pdpt | kTableMappingFlags;

	for (uint64 i = 0; i < maxAddress; i += 0x40000000) {
		pageDir = (uint64*)mmu_allocate_page();
		memset(pageDir, 0, B_PAGE_SIZE);
		pdpt[i / 0x40000000] = (addr_t)pageDir | kTableMappingFlags;

		for (uint64 j = 0; j < 0x40000000; j += 0x200000) {
			pageDir[j / 0x200000] = (i + j) | kLargePageMappingFlags;
		}
	}

	// Allocate tables for the kernel mappings.

	pdpt = (uint64*)mmu_allocate_page();
	memset(pdpt, 0, B_PAGE_SIZE);
	sv39[511] = (addr_t)pdpt | kTableMappingFlags;

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
			KERNEL_LOAD_BASE_64_BIT + (i * B_PAGE_SIZE), &phys) != B_OK) {
			continue;
		}

		pageTable[i % 512] = (addr_t)phys | kPageMappingFlags;
	}
	#endif

	return (uint64)sv39;
}
