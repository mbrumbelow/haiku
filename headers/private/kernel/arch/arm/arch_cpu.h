/*
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef _KERNEL_ARCH_ARM_CPU_H
#define _KERNEL_ARCH_ARM_CPU_H


#define CPU_MAX_CACHE_LEVEL 8
#define CACHE_LINE_SIZE 64
	// TODO: Could be 32-bits sometimes?


#define isb() __asm__ __volatile__("isb" : : : "memory")
#define dsb() __asm__ __volatile__("dsb" : : : "memory")
#define dmb() __asm__ __volatile__("dmb" : : : "memory")

#define set_ac()
#define clear_ac()


#ifndef _ASSEMBLER

#include <arch/arm/arch_thread_types.h>
#include <kernel.h>

/**! Values for arch_cpu_info.arch */
enum {
	ARCH_ARM_PRE_ARM7,
	ARCH_ARM_v3,
	ARCH_ARM_v4,
	ARCH_ARM_v4T,
	ARCH_ARM_v5,
	ARCH_ARM_v5T,
	ARCH_ARM_v5TE,
	ARCH_ARM_v5TEJ,
	ARCH_ARM_v6,
	ARCH_ARM_v7
};

typedef struct arch_cpu_info {
	/* For a detailed interpretation of these values,
	   see "The System Control coprocessor",
	   "Main ID register" in your ARM ARM */
	int implementor;
	int part_number;
	int revision;
	int variant;
	int arch;
} arch_cpu_info;

#ifdef __cplusplus
extern "C" {
#endif


#define get_special_reg(name, cp, opc1, crn, crm, opc2) \
	static inline uint32 \
	arm_get_##name(void) \
	{ \
		uint32 res; \
		asm volatile ("mrc " #cp ", " #opc1 ", %0, " #crn ", " #crm ", " #opc2 : "=r" (res)); \
		return res; \
	}


#define set_special_reg(name, cp, opc1, crn, crm, opc2) \
	static inline void \
	arm_set_##name(uint32 val) \
	{ \
		asm volatile ("mcr " #cp ", " #opc1 ", %0, " #crn ", " #crm ", " #opc2 :: "r" (val)); \
	}


/* CP15 c1, System Control Register */
get_special_reg(sctlr, p15, 0, c1, c0, 0)
set_special_reg(sctlr, p15, 0, c1, c0, 0)

/* CP15 c2, Translation table support registers */
get_special_reg(ttbr0, p15, 0, c2, c0, 0)
set_special_reg(ttbr0, p15, 0, c2, c0, 0)
get_special_reg(ttbr1, p15, 0, c2, c0, 1)
set_special_reg(ttbr1, p15, 0, c2, c0, 1)
get_special_reg(ttbcr, p15, 0, c2, c0, 2)
set_special_reg(ttbcr, p15, 0, c2, c0, 2)

/* CP15 c5 and c6, Memory system fault registers */
get_special_reg(dfsr, p15, 0, c5, c0, 0)
get_special_reg(ifsr, p15, 0, c5, c0, 1)
get_special_reg(dfar, p15, 0, c6, c0, 0)
get_special_reg(ifar, p15, 0, c6, c0, 2)

/* CP15 c13, Process, context and thread ID registers */
get_special_reg(tpidruro, p15, 0, c13, c0, 3)
set_special_reg(tpidruro, p15, 0, c13, c0, 3)
get_special_reg(tpidrprw, p15, 0, c13, c0, 4)
set_special_reg(tpidrprw, p15, 0, c13, c0, 4)


static inline addr_t
arm_get_fp(void)
{
	uint32 res;
	asm volatile ("mov %0, fp": "=r" (res));
	return res;
}


void arch_cpu_invalidate_TLB_page(addr_t page);

static inline void
arch_cpu_pause(void)
{
	// TODO: ARM Priority pause call
}


static inline void
arch_cpu_idle(void)
{
	uint32 Rd = 0;
	asm volatile("mcr p15, 0, %[c7format], c7, c0, 4"
		: : [c7format] "r" (Rd) );
}


#ifdef __cplusplus
};
#endif

#endif	// !_ASSEMBLER

#endif	/* _KERNEL_ARCH_ARM_CPU_H */
