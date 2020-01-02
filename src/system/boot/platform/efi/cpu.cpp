/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot/kernel_args.h>
#include <boot/stage2.h>
#include <arch/cpu.h>
#include <arch/x86/arch_cpu.h>


extern "C" void
cpu_init()
{
	calculate_cpu_conversion_factor(2);

	gKernelArgs.num_cpus = 1;
		// this will eventually be corrected later on
}
