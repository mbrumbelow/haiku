/*
 * Copyright 2007-2010, François Revol, revol@free.fr.
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2019-2021, Adrien Destugues, pulkomandy@pulkomandy.tk.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <KernelExport.h>
#include <kernel.h>
#include <vm/vm.h>
#include <vm/vm_priv.h>
#include <vm/VMAddressSpace.h>

#include "paging/ultrasparcII/SparcPagingMethodUltrasparcII.h"


#define TRACE_VM_TMAP
#ifdef TRACE_VM_TMAP
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


static union {
	uint64	align;
	char	ultrasparcII[sizeof(SparcPagingMethodUltrasparcII)];
} sPagingMethodBuffer;


status_t
arch_vm_translation_map_init(kernel_args *args,
	VMPhysicalPageMapper** _physicalPageMapper)
{
	TRACE("vm_translation_map_init: entry\n");

#ifdef TRACE_VM_TMAP
	TRACE("physical memory ranges:\n");
	for (uint32 i = 0; i < args->num_physical_memory_ranges; i++) {
		phys_addr_t start = args->physical_memory_range[i].start;
		phys_addr_t end = start + args->physical_memory_range[i].size;
		TRACE("  %#10" B_PRIxPHYSADDR " - %#10" B_PRIxPHYSADDR "\n", start,
			end);
	}

	TRACE("allocated physical ranges:\n");
	for (uint32 i = 0; i < args->num_physical_allocated_ranges; i++) {
		phys_addr_t start = args->physical_allocated_range[i].start;
		phys_addr_t end = start + args->physical_allocated_range[i].size;
		TRACE("  %#10" B_PRIxPHYSADDR " - %#10" B_PRIxPHYSADDR "\n", start,
			end);
	}

	TRACE("allocated virtual ranges:\n");
	for (uint32 i = 0; i < args->num_virtual_allocated_ranges; i++) {
		addr_t start = args->virtual_allocated_range[i].start;
		addr_t end = start + args->virtual_allocated_range[i].size;
		TRACE("  %#10" B_PRIxADDR " - %#10" B_PRIxADDR "\n", start, end);
	}
#endif

	// TODO detect CPU type and create the correct paging method
	// UltraSparct II: 40 bit physical address
	// UltraSparc III (JPS1): 42 bit physical address
	// UltraSparc T2 (UA2005): 55 bit physical address
	// The layout of the TLB entries changes a bit in each version
	dprintf("using Ultrasparc II paging\n");
	gSparcPagingMethod = new(&sPagingMethodBuffer) SparcPagingMethodUltrasparcII;

	return gSparcPagingMethod->Init(args, _physicalPageMapper);
}


status_t
arch_vm_translation_map_init_post_sem(kernel_args *args)
{
	TRACE("%s not implemented!\n", __func__);
	return B_NOT_SUPPORTED;
}


status_t
arch_vm_translation_map_init_post_area(kernel_args *args)
{
	TRACE("vm_translation_map_init_post_area: entry\n");
	return B_NOT_SUPPORTED;
}


status_t
arch_vm_translation_map_early_map(kernel_args *args, addr_t va, phys_addr_t pa,
	uint8 attributes, phys_addr_t (*get_free_page)(kernel_args *))
{
	//TRACE("early_tmap: entry pa 0x%lx va 0x%lx\n", pa, va);
	return gSparcPagingMethod->MapEarly(args, va, pa, attributes, get_free_page);
}


status_t
arch_vm_translation_map_create_map(bool kernel, VMTranslationMap** _map)
{
	TRACE("%s not implemented!\n", __func__);
	return B_NOT_SUPPORTED;
}


bool
arch_vm_translation_map_is_kernel_page_accessible(addr_t virtualAddress,
	uint32 protection)
{
	TRACE("%s not implemented!\n", __func__);
	return false;
}

