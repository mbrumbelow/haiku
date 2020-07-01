/*
 * Copyright 2004-2020, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot/kernel_args.h>
#include <boot/platform.h>
#include <boot/stage2.h>
#include <arch/cpu.h>
#include <boot/arch/x86/arch_cpu.h>


extern "C" status_t
arch_processor_init()
{
	// Nothing really to init on x86_64
	return B_OK;
}


extern "C" void
arch_ucode_load(BootVolume& volume)
{
	ucode_load(volume);
}
