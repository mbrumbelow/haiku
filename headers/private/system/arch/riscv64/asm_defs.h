/*
 * Copyright 2008, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 */
#ifndef SYSTEM_ARCH_RISCV64_ASM_DEFS_H
#define SYSTEM_ARCH_RISCV64_ASM_DEFS_H


/* include arch version helpers */
//#include <arch_riscv64_version.h>

#define SYMBOL(name)			.global name; name
#define SYMBOL_END(name)		1: .size name, 1b - name
#define STATIC_FUNCTION(name)	.type name, %function; name
#define FUNCTION(name)			.global name; .type name, %function; name
#define FUNCTION_END(name)		1: .size name, 1b - name

// Status register flags
#define ARCH_SR_SIE		0x00000002 // Supervisor Interrupt Enable
#define ARCH_SR_SPIE		0x00000020 // Previous Supervisor Interrupt En
#define ARCH_SR_SPP		0x00000100 // Previously Supervisor
#define ARCH_SR_SUM		0x00040000 // Supervisor may access user memory

#define ARCH_SR_FS		0x00006000 // Floating Point Status
#define ARCH_SR_FS_OFF		0x00000000
#define ARCH_SR_FS_INITIAL	0x00002000
#define ARCH_SR_FS_CLEAN	0x00004000
#define ARCH_SR_FS_DIRTY	0x00006000

#define ARCH_SR_XS		0x00018000 // Extension Status
#define ARCH_SR_XS_OFF		0x00000000
#define ARCH_SR_XS_INITIAL	0x00008000
#define ARCH_SR_XS_CLEAN	0x00010000
#define ARCH_SR_XS_DIRTY	0x00018000

#define ARCH_SR_SD		0x8000000000000000 // FS/XS dirty

// Interrupt Enable and Interrupt Pending
#define ARCH_SIE_SSIE		0x00000002 // Software Interrupt Enable
#define ARCH_SIE_STIE		0x00000020 // Timer Interrupt Enable
#define ARCH_SIE_SEIE		0x00000200 // External Interrupt Enable


#endif	/* SYSTEM_ARCH_RISCV64_ASM_DEFS_H */
