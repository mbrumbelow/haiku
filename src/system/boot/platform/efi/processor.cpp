/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot/kernel_args.h>
#include <boot/platform.h>
#include <boot/stage2.h>
#include <arch/cpu.h>

#include "arch_processor.h"


void
processor_init()
{
	gKernelArgs.num_cpus = 1;
		// this will eventually be corrected later on

	arch_processor_init();
}


extern "C" void
platform_load_ucode(BootVolume& volume)
{
	arch_ucode_load(volume);
}
