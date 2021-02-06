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

#include "mmu.h"
#include "efi_platform.h"


// RISC-V Privileged Architectures V1.10, section 4.1.12
//
// RV64 Supervisor Address Transaltion and Protection (satp)
// Physical Page Number of the root page table
// (supervisor physical address divided by 4KiB)
#define RV64_SATP_PPN_MASK		0x00000FFFFFFFFFFF

// Address Space Identifier
// (facilitates address-transation fences on a per-address-space basis)
#define RV64_SATP_ASID_SHIFT	0x2C
#define RV64_SATP_ASID_MASK		0x0FFFF00000000000

// Mode (selects the current address-translation scheme)
#define RV64_SATP_MODE_SHIFT	0x3C
#define RV64_SATP_MODE_MASK		0xF000000000000000
#define RV64_SATP_MODE_NONE		0x0
#define RV64_SATP_MODE_SV39		0x8
#define RV64_SATP_MODE_SV48		0x9

#define SV39_PTE_FLAGS			0x00000000000003FF
#define SV39_PTE_PPN			0x003FFFFFFFFFFC00
#define PPN_TO_ADDR(ppn)		(ppn << 12)
#define ADDR_TO_PPN(addr)		(addr >> 12)
#define SV39_PTE_TO_ADDR(pte)	PPN_TO_ADDR((((uint64_t)pte & SV39_PTE_PPN) >> 10))
#define SV39_PTE_TO_FLAGS(pte)	((uint64_t)pte & SV39_PTE_FLAGS)
#define SV39_ADDR_TO_PTE(addr)	(ADDR_TO_PPN((uint64_t)addr) << 10)

// Page Flags
#define RISCV_PAGE_VALID		(1 << 0)
#define RISCV_PAGE_READ			(1 << 1)
#define RISCV_PAGE_WRITE		(1 << 2)
#define RISCV_PAGE_EXECUTE		(1 << 3)
#define RISCV_PAGE_VALID_RWX	(RISCV_PAGE_VALID | RISCV_PAGE_READ \
									| RISCV_PAGE_WRITE | RISCV_PAGE_EXECUTE)
#define RISCV_PAGE_USER			(1 << 4)
#define RISCV_PAGE_GLOBAL		(1 << 5)
#define RISCV_PAGE_ACCESSED		(1 << 6)
#define RISCV_PAGE_DIRTY		(1 << 7)


// Accessed and Dirty leaf pages due to lack of an exception
// handler early on.
static const uint64 kLeafPageFlags = RISCV_PAGE_VALID_RWX
	| RISCV_PAGE_ACCESSED | RISCV_PAGE_DIRTY;
static const uint64 kPageMappingFlags = RISCV_PAGE_VALID;


static void
arch_mmu_dumpflags(uint64_t flags)
{
	char info[] = "--------\0";
	if ((flags & RISCV_PAGE_VALID) != 0)
		info[0] = 'V';
	if ((flags & RISCV_PAGE_READ) != 0)
		info[1] = 'R';
	if ((flags & RISCV_PAGE_WRITE) != 0)
		info[2] = 'W';
	if ((flags & RISCV_PAGE_EXECUTE) != 0)
		info[3] = 'X';
	if ((flags & RISCV_PAGE_USER) != 0)
		info[4] = 'U';
	if ((flags & RISCV_PAGE_GLOBAL) != 0)
		info[5] = 'G';
	if ((flags & RISCV_PAGE_ACCESSED) != 0)
		info[6] = 'A';
	if ((flags & RISCV_PAGE_DIRTY) != 0)
		info[7] = 'D';
	dprintf("%s", info);
}


static void
arch_mmu_dumpmap(uint64_t* pagetable, int level)
{
	if (level == 2)
		dprintf("Sv39 Page Table @ %p\n", pagetable);

	for(int k = 0; k < 512; k++) {
		uint64_t pte = pagetable[k];
		if ((pte & RISCV_PAGE_VALID) != 0) {
			dprintf("  %s%s%03d: pte 0x%016lx pa 0x%016lx (",
				level < 2 ? ".." : "",
				level < 1 ? ".." : "",
				k, pte, SV39_PTE_TO_ADDR(pte));
			arch_mmu_dumpflags(SV39_PTE_TO_FLAGS(pte));
			dprintf(")\n");
			// If this is a leaf, we're at the end
			if (SV39_PTE_TO_FLAGS(pte) == kLeafPageFlags)
				continue;
			int next_level = level - 1;
			if (level > 0 && SV39_PTE_TO_FLAGS(pte) & kLeafPageFlags) {
				uint64_t* pt = (uint64_t*)SV39_PTE_TO_ADDR(pte);
				arch_mmu_dumpmap(pt, next_level);
			}
		}
	}
}


void
arch_mmu_init()
{
	// Stub
}


// Generate the final satp with our new sv39 page directory
uint64_t
arch_mmu_generate_satp(uint64_t pageDirAddr)
{
	dprintf("%s called, sv39 @ 0x%" B_PRIx64 "\n", __func__, pageDirAddr);

	uint64_t satp = 0x0;

	// Set sv39 mode
	satp |= ((uint64_t)RV64_SATP_MODE_SV39 << RV64_SATP_MODE_SHIFT);
	satp |= ADDR_TO_PPN(pageDirAddr);

	// We leave ASID 0 for now

	dprintf("  new satp: 0x%" B_PRIx64 "\n", satp);
	return satp;
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
}

uint64_t
arch_mmu_generate_post_efi_page_tables(size_t memory_map_size,
	efi_memory_descriptor *memory_map, size_t descriptor_size,
	uint32_t descriptor_version)
{
	uint64_t *pageDirL2;
		// 512GiB L2 pages
	uint64_t *pageDirL1;
		// 1GiB L1 pages
	uint64_t *pageDirL0;
		// 2MiB L0 pages

	// Allocate the top level page table (sv39 assumed)
	pageDirL2 = NULL;
	if (platform_allocate_region((void**)&pageDirL2,
		B_PAGE_SIZE, 0, false) != B_OK) {
		panic("Failed to allocate page directory");
	}
	gKernelArgs.arch_args.phys_pgdir = (uint64_t)(addr_t)pageDirL2;
	memset(pageDirL2, 0, B_PAGE_SIZE);
	platform_bootloader_address_to_kernel_address(pageDirL2,
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

	// Sv39 supports a maximum virtual address space of 512GiB.
	if (maxAddress / 0x40000000 > 512)
		panic("Sv39 currently supports up to 512GiB of RAM!");

	// Create page tables for the physical map area. Also map this page directory
	// temporarily at the bottom of the address space so that we are identity
	// mapped.

	pageDirL1 = (uint64*)mmu_allocate_page();
	memset(pageDirL1, 0, B_PAGE_SIZE);
	pageDirL2[510] = SV39_ADDR_TO_PTE(pageDirL1) | kPageMappingFlags;
	pageDirL2[0] = SV39_ADDR_TO_PTE(pageDirL1) | kPageMappingFlags;

	dprintf("Mapping 0x%016x - 0x%016" B_PRIx64 " as physical ram\n",
		0x0, maxAddress);
	// Map physical memory by 1 GiB chunks
	for (uint64 i = 0; i < maxAddress; i += 0x40000000) {
		pageDirL1[i / 0x40000000] = SV39_ADDR_TO_PTE(i) | kLeafPageFlags;
		#if 0
		pageDirL0 = (uint64*)mmu_allocate_page();
		memset(pageDirL0, 0, B_PAGE_SIZE);
		pageDirL1[i / 0x40000000]
			= SV39_ADDR_TO_PTE(pageDirL0) | kPageMappingFlags;
		// Map down to 2 MiB to tall
		for (uint64 j = 0; j < 0x40000000; j += 0x200000) {
			pageDirL0[j / 0x200000]
				= SV39_ADDR_TO_PTE((i + j)) | kLeafPageFlags;
		}
		#endif
	}

        // Allocate tables for the kernel mappings.
	pageDirL1 = (uint64*)mmu_allocate_page();
	memset(pageDirL1, 0, B_PAGE_SIZE);
	pageDirL2[511] = SV39_ADDR_TO_PTE(pageDirL1) | kPageMappingFlags;

	pageDirL0 = (uint64*)mmu_allocate_page();
	memset(pageDirL0, 0, B_PAGE_SIZE);
	pageDirL1[510] = SV39_ADDR_TO_PTE(pageDirL0) | kPageMappingFlags;

	for (uint32 i = 0; i < gKernelArgs.virtual_allocated_range[0].size
			/ B_PAGE_SIZE; i++) {
		if ((i % 512) == 0) {
			pageDirL1 = (uint64*)mmu_allocate_page();
			memset(pageDirL1, 0, B_PAGE_SIZE);
			pageDirL0[i / 512]
				= SV39_ADDR_TO_PTE(pageDirL1) | kPageMappingFlags;
		}

		// Get the physical address to map.
		void *phys;
		if (platform_kernel_address_to_bootloader_address(
			KERNEL_LOAD_BASE_64_BIT + (i * B_PAGE_SIZE), &phys) != B_OK) {
			continue;
		}

		pageDirL0[i % 512] = SV39_ADDR_TO_PTE(phys) | kLeafPageFlags;
	}

	arch_mmu_dumpmap(pageDirL2, 2);

	return (uint64)pageDirL2;
}
