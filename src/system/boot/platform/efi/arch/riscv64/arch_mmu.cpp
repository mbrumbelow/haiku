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
#define RV64_SATP_PPN_SHIFT	0x0
#define RV64_SATP_PPN_MASK	0x00000FFFFFFFFFFF

// Address Space Identifier
// (facilitates address-transation fences on a per-address-space basis)
#define RV64_SATP_ASID_SHIFT	0x2C
#define RV64_SATP_ASID_MASK	0x0FFFF00000000000

// Mode (selects the current address-translation scheme)
#define RV64_SATP_MODE_SHIFT	0x3C
#define RV64_SATP_MODE_MASK	0xF000000000000000
#define RV64_SATP_MODE_NONE	0x0
#define RV64_SATP_MODE_SV39	0x8
#define RV64_SATP_MODE_SV48	0x9

#define SV39_PTE_TO_PTR(pte)	(uint64_t*)((uint64_t)pte >> 10)
#define SV39_PTE_TO_FLAG(pte)	(uint64_t)((uint64_t)pte & 0xFF3)
#define SV39_PTR_TO_PTE(ptr)	(uint64_t)((uint64_t)ptr << 10)

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


static void
arch_mmu_dumpflags(uint64_t flags)
{
	char info[8];
	for (size_t i = 0; i < sizeof(info); i++)
		info[i] = '-';
	if ((flags & RISCV_PAGE_VALID) != 0)
		info[0] = 'V';
	if ((flags & RISCV_PAGE_R) != 0)
		info[1] = 'R';
	if ((flags & RISCV_PAGE_W) != 0)
		info[2] = 'W';
	if ((flags & RISCV_PAGE_EX) != 0)
		info[3] = 'E';
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
			dprintf("  %s%s%03d: pte 0x%lx pa %p (",
				level < 2 ? ".." : "",
				level < 1 ? ".." : "",
				k, pte, SV39_PTE_TO_PTR(pte));
			arch_mmu_dumpflags(SV39_PTE_TO_FLAG(pte));
			dprintf(")\n");
			int next_level = level - 1;
			if (level > 0) {
				uint64_t* pt = SV39_PTE_TO_PTR(pte);
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


// Generate the final satp with our new sv39 mmu
uint64_t
arch_mmu_generate_satp(uint64_t sv39)
{
	dprintf("%s called sv39: 0x%" B_PRIx64 "\n", __func__, sv39);

	uint64_t satp = 0;
	__asm__	__volatile__ ("csrr %0, satp" : "=r"(satp) : : );
	dprintf("  current satp: 0x%" B_PRIx64 "\n", satp);

	// Set sv39 mode
	satp = (satp & ~RV64_SATP_MODE_MASK)
		| ((uint64_t)RV64_SATP_MODE_SV39 << RV64_SATP_MODE_SHIFT);
	// Our new sv39 table
	satp = (satp & ~RV64_SATP_PPN_MASK)
		| (sv39 << RV64_SATP_PPN_SHIFT);

	// TODO: Do we need to set / change ASID?

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

	// Important.  Make sure supervisor threads can fault on read only pages...
	//asm("mov %%rax, %%cr0" : : "a" ((1 << 31) | (1 << 16) | (1 << 5) | 1));
}

uint64_t
arch_mmu_generate_post_efi_page_tables(size_t memory_map_size,
	efi_memory_descriptor *memory_map, size_t descriptor_size,
	uint32_t descriptor_version)
{
	uint64_t *teraPage;
		// 512GiB terapages
	uint64_t *gigaPage;
		// 1GiB gigapages
	uint64_t *megaPage;
		// 2MiB megapages

	// Allocate the top level PTE (sv39 assumed)
	teraPage = NULL;
	if (platform_allocate_region((void**)&teraPage,
		B_PAGE_SIZE, 0, false) != B_OK) {
		panic("Failed to allocate page directory");
	}
	gKernelArgs.arch_args.phys_pgdir = (uint64_t)(addr_t)teraPage;
	memset(teraPage, 0, B_PAGE_SIZE);
	platform_bootloader_address_to_kernel_address(teraPage,
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
	// Switching to Sv48 allows up to 256TiB, but uses more memory
	// for page tables.
	if (maxAddress / 0x40000000 > 512)
		panic("Sv39 currently supports up to 512GiB of RAM!");

	// Create page tables for the physical map area. Also map this page directory
	// temporarily at the bottom of the address space so that we are identity
	// mapped.

	gigaPage = (uint64*)mmu_allocate_page();
	memset(gigaPage, 0, B_PAGE_SIZE);
	teraPage[510] = SV39_PTR_TO_PTE(gigaPage) | kTableMappingFlags;
	teraPage[0] = SV39_PTR_TO_PTE(gigaPage) | kTableMappingFlags;

	// Map physical memory by 1 GiB chunks
	for (uint64 i = 0; i < maxAddress; i += 0x40000000) {
		megaPage = (uint64*)mmu_allocate_page();
		memset(megaPage, 0, B_PAGE_SIZE);
		gigaPage[i / 0x40000000]
			= SV39_PTR_TO_PTE(megaPage) | kTableMappingFlags;
		// Map down to 2 MiB to tall
		for (uint64 j = 0; j < 0x40000000; j += 0x200000) {
			megaPage[j / 0x200000]
				= SV39_PTR_TO_PTE((i + j)) | kTableMappingFlags;
		}
	}

	// Allocate tables for the kernel mappings.
	gigaPage = (uint64*)mmu_allocate_page();
	memset(gigaPage, 0, B_PAGE_SIZE);
	teraPage[511] = SV39_PTR_TO_PTE(gigaPage) | kTableMappingFlags;

	megaPage = (uint64*)mmu_allocate_page();
	memset(megaPage, 0, B_PAGE_SIZE);
	gigaPage[510] = SV39_PTR_TO_PTE(megaPage) | kTableMappingFlags;

	for (uint32 i = 0; i < gKernelArgs.virtual_allocated_range[0].size
			/ B_PAGE_SIZE; i++) {
		if ((i % 512) == 0) {
			gigaPage = (uint64*)mmu_allocate_page();
			memset(gigaPage, 0, B_PAGE_SIZE);
			megaPage[i / 512]
				= SV39_PTR_TO_PTE(gigaPage) | kTableMappingFlags;
		}

		// Get the physical address to map.
		void *phys;
		if (platform_kernel_address_to_bootloader_address(
			KERNEL_LOAD_BASE_64_BIT + (i * B_PAGE_SIZE), &phys) != B_OK) {
			continue;
		}

		megaPage[i % 512] = SV39_PTR_TO_PTE(phys) | kPageMappingFlags;
	}

	arch_mmu_dumpmap(teraPage, 2);

	return (uint64)teraPage;
}
