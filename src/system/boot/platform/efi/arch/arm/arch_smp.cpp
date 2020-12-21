/*
 * Copyright 2019-2020, Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
*/


#include "arch_smp.h"

#include <string.h>

#include <KernelExport.h>

#include <kernel.h>
#include <safemode.h>
#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/menu.h>

#include "mmu.h"

//#define TRACE_SMP
#ifdef TRACE_SMP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


int
arch_smp_get_current_cpu(void)
{
	// One cpu for now.
	return 0;
}


void
arch_smp_init_other_cpus(void)
{
	// XXX: One cpu for now.
	gKernelArgs.num_cpus = 1;

	if (get_safemode_boolean(B_SAFEMODE_DISABLE_SMP, false)) {
		// SMP has been disabled!
		TRACE(("smp disabled per safemode setting\n"));
		gKernelArgs.num_cpus = 1;
	}

	TRACE(("smp: found %" B_PRId32 " cpu%s\n", gKernelArgs.num_cpus,
		gKernelArgs.num_cpus != 1 ? "s" : ""));

	for (uint32 i = 1; i < gKernelArgs.num_cpus; i++) {
		// create a final stack the trampoline code will put the ap processor on
		void* stack = NULL;
		const size_t size = KERNEL_STACK_SIZE + KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE;
		if (platform_allocate_region(&stack, size, 0, false) != B_OK) {
			panic("Unable to allocate AP stack");
		}
		memset(stack, 0, size);
		gKernelArgs.cpu_kstack[i].start = fix_address((uint64_t)stack);
		gKernelArgs.cpu_kstack[i].size = size;
	}

}


void
arch_smp_boot_other_cpus(uint32 pml4, uint64 kernel_entry)
{
	// One cpu for now.
}


void
arch_smp_add_safemode_menus(Menu *menu)
{
	MenuItem *item;

	if (gKernelArgs.num_cpus < 2)
		return;

	item = new(nothrow) MenuItem("Disable SMP");
	menu->AddItem(item);
	item->SetData(B_SAFEMODE_DISABLE_SMP);
	item->SetType(MENU_ITEM_MARKABLE);
	item->SetHelpText("Disables all but one CPU core.");
}


void
arch_smp_init(void)
{
	// One cpu for now.
}
