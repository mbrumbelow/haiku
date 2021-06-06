/* Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk.
 * Distributed under the terms of the MIT License.
 */


#include <arch/platform.h>
#include <boot/kernel_args.h>
#include <Htif.h>
#include <Plic.h>
#include <Clint.h>
#include <platform/sbi/sbi_syscalls.h>

#include <debug.h>


uint32 gPlatform1;
uint32 gPlatform2;

void* gFDT = NULL;

HtifRegs  *volatile gHtifRegs  = (HtifRegs *volatile)0x40008000;
PlicRegs  *volatile gPlicRegs;
ClintRegs *volatile gClintRegs;


status_t
arch_platform_init(struct kernel_args *args)
{
	gPlatform1 = args->arch_args.platform1;
	gPlatform2 = args->arch_args.platform2;

	debug_early_boot_message("platform1: ");
	switch (gPlatform1) {
		case kPlatform1Riscv: debug_early_boot_message("riscv"); break;
		case kPlatform1Sbi:   debug_early_boot_message("sbi"); break;
		default: debug_early_boot_message("?"); break;
	}
	debug_early_boot_message("\n");

	debug_early_boot_message("platform2: ");
	switch (gPlatform1) {
		case kPlatform2Riscv: debug_early_boot_message("riscv"); break;
		case kPlatform2Efi:   debug_early_boot_message("efi"); break;
		case kPlatform2UBoot: debug_early_boot_message("uBoot"); break;
		default: debug_early_boot_message("?"); break;
	}
	debug_early_boot_message("\n");


	gFDT = args->arch_args.fdt;

	gHtifRegs  = (HtifRegs  *volatile)args->arch_args.htif.start;
	gPlicRegs  = (PlicRegs  *volatile)args->arch_args.plic.start;
	gClintRegs = (ClintRegs *volatile)args->arch_args.clint.start;

	return B_OK;
}


status_t
arch_platform_init_post_vm(struct kernel_args *kernelArgs)
{
	if (gPlatform1 == kPlatform1Sbi) {
		sbiret res;
		res = sbi_get_spec_version();
		dprintf("SBI spec version: %#lx\n", res.value);
		res = sbi_get_impl_id();
		dprintf("SBI implementation ID: %#lx\n", res.value);
		res = sbi_get_impl_version();
		dprintf("SBI implementation version: %#lx\n", res.value);
		res = sbi_get_mvendorid();
		dprintf("SBI vendor ID: %#lx\n", res.value);
		res = sbi_get_marchid();
		dprintf("SBI arch ID: %#lx\n", res.value);
	}
	return B_OK;
}


status_t
arch_platform_init_post_thread(struct kernel_args *kernelArgs)
{
	return B_OK;
}
