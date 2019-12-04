/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "cpu.h"

#include "efi_platform.h"

#include <OS.h>
#include <boot/platform.h>
#include <boot/stdio.h>
#include <boot/kernel_args.h>
#include <boot/stage2.h>
#include <arch/cpu.h>
#include <arch_kernel.h>
#include <arch_system_info.h>

#include <string.h>


//#define TRACE_CPU
#ifdef TRACE_CPU
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


extern "C" uint64 rdtsc();

extern uint32 gTimeConversionFactor;


//	#pragma mark -


extern "C" bigtime_t
system_time()
{
	uint64 lo, hi;
	asm("rdtsc": "=a"(lo), "=d"(hi));
	return ((lo * gTimeConversionFactor) >> 32) + hi * gTimeConversionFactor;
}


extern "C" void
cpu_init()
{
	calculate_cpu_conversion_factor(2);

	gKernelArgs.num_cpus = 1;
		// this will eventually be corrected later on
}
