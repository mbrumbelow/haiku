/*
 * Copyright (c) 2021 Maxime Villard, m00nbsd.net
 * Copyright (c) 2021 The DragonFly Project.
 * All rights reserved.
 *
 * This code is part of the NVMM hypervisor.
 *
 * This code is derived from software contributed to The DragonFly Project
 * by Aaron LI <aly@aaronly.me>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

// Assume there would be netBSD and DragonFlyBSD specific code here too
// includes
#ifdef __HAIKU__
#include <arch/x86/arch_cpu.h>
#include <cpu.h>

#include <SupportDefs.h>
#endif

#ifdef __HAIKU__
#define TRACE_ALWAYS(a...) dprintf(a)
#endif
// functions
#ifdef __HAIKU__
#define rdmsr		x86_read_msr
#define os_printf	TRACE_ALWAYS
#define x86_get_cr0	x86_read_cr0
#define x86_get_cr4	x86_read_cr4
#endif

// constants
#ifdef __HAIKU__
#define CR0_PG		CR0_PAGING_ENABLE
#define CR0_PE		CR0_PROTECTED_MODE_ENABLE
#define CR4_VMXE	IA32_CR4_VMXE
#endif
