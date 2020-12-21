/*
 * Copyright 2019-2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *   Alexander von Gluck IV <kallisti5@unixzen.com>
 */


#include <boot/addr_range.h>
#include <boot/platform.h>
#include <boot/stage2.h>
#include <kernel/kernel.h>

extern "C" {
#include <fdt.h>
#include <libfdt.h>
#include <libfdt_env.h>
};

#include "efi_platform.h"
#include "fdt_support.h"


#define TRACE_DTB
#ifdef TRACE_DTB
#   define TRACE(x...) dprintf("efi/fdt: " x)
#else
#   define TRACE(x...) ;
#endif

#define ERROR(x...) dprintf("efi/fdt: " x)


extern "C" void
dtb_init()
{
	efi_guid dtb = DEVICE_TREE_GUID;
	efi_configuration_table *table = kSystemTable->ConfigurationTable;
	size_t entries = kSystemTable->NumberOfTableEntries;

	// Try to find an FDT
	for (uint32 i = 0; i < entries; i++) {
		void* dtbPtr = NULL;

		efi_guid vendor = table[i].VendorGuid;

		if (vendor.data1 == dtb.data1
			&& vendor.data2 == dtb.data2
			&& vendor.data3 == dtb.data3
			&& strncmp((char *)vendor.data4,
				(char *)dtb.data4, 8) == 0) {

			dtbPtr = (void*)(table[i].VendorTable);

			// TODO: More checks of FDT?
			if (dtbPtr == NULL) {
				TRACE("Invalid FDT from UEFI table %d\n", i);
				break;
			}

			TRACE("Valid FDT from UEFI table %d\n", i);

			// pack into proper location if the architecture cares
			#ifdef __ARM__
			size_t fdtSize = fdt_totalsize(dtbPtr);
			gKernelArgs.arch_args.fdt = kernel_args_malloc(fdtSize);
			if (gKernelArgs.arch_args.fdt != NULL) {
				memcpy(gKernelArgs.arch_args.fdt, dtbPtr,
					fdtSize);
			} else
				ERROR("unable to malloc for fdt!\n");
			#endif
		}
	}
}
