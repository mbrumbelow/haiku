/*
 * Copyright 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include "arch/x86/arch_altcodepatch.h"

#include <stdlib.h>
#include <string.h>

#include <KernelExport.h>

#include <kernel.h>
#include <vm_defs.h>



typedef struct altcodepatch {
	uint32 kernel_offset;
	uint16 length;
	uint16 tag;
} altcodepatch;


extern altcodepatch altcodepatch_begin;
extern altcodepatch altcodepatch_end;


void
arch_altcodepatch_replace(uint16 tag, void* newcodepatch, size_t length)
{
	uint32 count = 0;
	uint32 kernelProtection =  B_KERNEL_READ_AREA | B_KERNEL_EXECUTE_AREA;

	for (altcodepatch *patch = &altcodepatch_begin; patch < &altcodepatch_end;
		patch++) {
		if (patch->tag != tag)
			continue;
		void* address = (void*)(KERNEL_LOAD_BASE + patch->kernel_offset);
		if (patch->length < length)
			panic("can't copy patch: new code is too long\n");
		area_id area = area_for(address);
		if (area < 0)
			continue;
		// we need to write to the text area
		set_area_protection(area, kernelProtection | B_KERNEL_WRITE_AREA);
		memcpy(address, newcodepatch, length);
		// disable write after patch
		set_area_protection(area, kernelProtection);
		count++;
	}
	dprintf("arch_altcodepatch_replace found %d altcodepatches\n", count);
}



