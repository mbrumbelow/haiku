/* 
 * Copyright 2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		François Revol <revol@free.fr>
 *
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>

#include <boot/stage2.h>
#include <arch/smp.h>
#include <debug.h>
#include <int.h>

#include <cpu.h>
#include <platform/sbi/sbi_syscalls.h>


extern uint32 gPlatform1;
extern uint32 gPlatform2;


status_t
arch_smp_init(kernel_args *args)
{
	dprintf("arch_smp_init()\n");
	return B_OK;
}


status_t
arch_smp_per_cpu_init(kernel_args *args, int32 cpuId)
{
	return B_OK;
}


void
arch_smp_send_multicast_ici(CPUSet& cpuSet)
{
	switch (gPlatform1) {
	case kPlatform1Sbi: {
		int32 cpuCount = smp_get_num_cpus();
		for (int32 i = 0; i < cpuCount; i++) {
			if (cpuSet.GetBit(i) && i != smp_get_current_cpu()) {
				// TODO: use bitset to send multiple IPI at once
				sbi_send_ipi((uint64)1 << gCPU[i].arch.hartId, 0);
			}
		}
		break;
	}
	default:
		dprintf("arch_smp_send_multicast_ici: not implemented\n");
	}
#if KDEBUG
	if (are_interrupts_enabled())
		panic("arch_smp_send_multicast_ici: called with interrupts enabled");
#endif
}


void
arch_smp_send_ici(int32 target_cpu)
{
	switch (gPlatform1) {
	case kPlatform1Sbi:
		// dprintf("arch_smp_send_ici(%" B_PRId32 ")\n", target_cpu);
		sbi_send_ipi((uint64)1 << gCPU[target_cpu].arch.hartId, 0);
		break;
	default:
		dprintf("arch_smp_send_ici: not implemented\n");
	}
}


void
arch_smp_send_broadcast_ici()
{
	switch (gPlatform1) {
	case kPlatform1Sbi:
		// dprintf("arch_smp_send_broadcast_ici()\n");
		sbi_send_ipi(0, -1);
		break;
	default:
		dprintf("arch_smp_send_broadcast_ici: not implemented\n");
	}
}
