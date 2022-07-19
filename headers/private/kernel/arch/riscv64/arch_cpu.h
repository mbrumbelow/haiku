/*
 * Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_RISCV64_CPU_H
#define _KERNEL_ARCH_RISCV64_CPU_H


#include <arch/riscv64/arch_thread_types.h>
#include <arch_cpu_defs.h>
#include <kernel.h>


#define CPU_MAX_CACHE_LEVEL	8
#define CACHE_LINE_SIZE		64

// Standard Extensions
#define STANDARD_EXT_A	0 << 1  // Atomic
#define STANDARD_EXT_B	1 << 1  // Reserved - Bit-Manipulation
#define STANDARD_EXT_C	2 << 1  // Compressed
#define STANDARD_EXT_D	3 << 1  // Double-Precision FP
#define STANDARD_EXT_E	4 << 1  // RV32E Base ISA
#define STANDARD_EXT_F	5 << 1  // Single-Precision FP
#define STANDARD_EXT_G	6 << 1  // Reserved
#define STANDARD_EXT_H	7 << 1  // Hypervisor
#define STANDARD_EXT_I	8 << 1  // RV32I/64I/128I Base ISA
#define STANDARD_EXT_J	9 << 1  // Reserved - Dynamic Trans. Lang.
#define STANDARD_EXT_K	10 << 1 // Reserved
#define STANDARD_EXT_L	11 << 1 // Reserved
#define STANDARD_EXT_M	12 << 1 // Integer Mul/Div
#define STANDARD_EXT_N	13 << 1 // Reserved - User-Level Irq
#define STANDARD_EXT_O	14 << 1 // Reserved
#define STANDARD_EXT_P	15 << 1 // Reserved - Packed-SIMD
#define STANDARD_EXT_Q	16 << 1 // Quad-Precision FP
#define STANDARD_EXT_R	17 << 1 // Reserved
#define STANDARD_EXT_S	18 << 1 // Supervisor Mode Implemented
#define STANDARD_EXT_T	19 << 1 // Reserved
#define STANDARD_EXT_U	20 << 1 // User mode
#define STANDARD_EXT_V	21 << 1 // Reserved - Vector
#define STANDARD_EXT_W	22 << 1 // Reserved
#define STANDARD_EXT_X	23 << 1 // Non-Standard Ext.
#define STANDARD_EXT_Y	24 << 1 // Reserved
#define STANDARD_EXT_Z	25 << 1 // Reserved

#define STANDARD_EXT_MASK 0x1FFFFFF


typedef struct arch_cpu_info {
	uint64	hartId;
	char	vendor[16];
	char	model_name[49];
	char	isa[32];
} arch_cpu_info;


static inline bool
get_ac()
{
	SstatusReg status(Sstatus());
	return status.sum != 0;
}


static inline void
set_ac()
{
	// TODO: Could be done atomically via CSRRS?
	SstatusReg status(Sstatus());
	status.sum = 1;
	SetSstatus(status.val);
}


static inline void
clear_ac()
{
	// TODO: Could be done atomically with CSRRC?
	SstatusReg status(Sstatus());
	status.sum = 0;
	SetSstatus(status.val);
}


#ifdef __cplusplus
extern "C" {
#endif


void __riscv64_setup_system_time(uint64 conversionFactor);


static inline void
arch_cpu_pause(void)
{
	// TODO: CPU pause
}


static inline void
arch_cpu_idle(void)
{
	Wfi();
}


#ifdef __cplusplus
}
#endif


#endif	/* _KERNEL_ARCH_RISCV64_CPU_H */
