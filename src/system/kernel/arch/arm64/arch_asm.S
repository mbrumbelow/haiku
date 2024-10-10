/*
 * Copyright 2019-2022 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#include <arch/arm/arch_cpu.h>
#include <asm_defs.h>
#include "asm_offsets.h"
#include "syscall_numbers.h"

.text

FUNCTION(_thread_exit_syscall):
	svc #((SYSCALL_EXIT_THREAD << 5) | 1)
FUNCTION_END(_thread_exit_syscall)

.macro EXCEPTION_ENTRY el
	// interrupts are automatically disabled by hardware

	// x19 is now the stack top, make room for IFRAME
	sub sp, sp, #(IFRAME_sizeof)

	stp	    x0,  x1, [sp, #(IFRAME_x + 0 * 8)]
	stp	    x2,  x3, [sp, #(IFRAME_x + 2 * 8)]
	stp	    x4,  x5, [sp, #(IFRAME_x + 4 * 8)]
	stp	    x6,  x7, [sp, #(IFRAME_x + 6 * 8)]
	stp	    x8,  x9, [sp, #(IFRAME_x + 8 * 8)]
	stp	   x10, x11, [sp, #(IFRAME_x + 10 * 8)]
	stp	   x12, x13, [sp, #(IFRAME_x + 12 * 8)]
	stp	   x14, x15, [sp, #(IFRAME_x + 14 * 8)]
	stp	   x16, x17, [sp, #(IFRAME_x + 16 * 8)]
	stp	   x18, x19, [sp, #(IFRAME_x + 18 * 8)]
	stp	   x20, x21, [sp, #(IFRAME_x + 20 * 8)]
	stp	   x22, x23, [sp, #(IFRAME_x + 22 * 8)]
	stp	   x24, x25, [sp, #(IFRAME_x + 24 * 8)]
	stp	   x26, x27, [sp, #(IFRAME_x + 26 * 8)]
	stp	   x28, fp,  [sp, #(IFRAME_x + 28 * 8)]
	str	   x30,      [sp, #(IFRAME_lr)]

.if \el == 0
	mrs x0, SP_EL0
.else
	// add sizeof back here to store original sp
	add x0, sp, #(IFRAME_sizeof)
.endif

	mrs x1, ELR_EL1
	mrs x2, SPSR_EL1
	mrs x3, ESR_EL1
	mrs x4, FAR_EL1

	str x0, [sp, #(IFRAME_sp)]
	stp x1, x2, [sp, #(IFRAME_elr)]
	stp x3, x4, [sp, #(IFRAME_esr)]
.endm

.macro EXCEPTION_RETURN el
	STATIC_FUNCTION(exception_return_\el):
		// disable interrupts before restoring ELR/SPSR/sp
		msr DAIFSet, #0xf

		// x18, x19 will be restored at the very end

		ldp lr, x18, [sp, #(IFRAME_lr)]
		ldp x10, x11, [sp, #(IFRAME_elr)]
.if \el == 0
		msr SP_EL0, x18
.endif
		msr SPSR_EL1, x11
		msr ELR_EL1, x10

		ldp     x0,  x1, [sp, #(IFRAME_x + 0 * 8)]
		ldp	    x2,  x3, [sp, #(IFRAME_x + 2 * 8)]
		ldp	    x4,  x5, [sp, #(IFRAME_x + 4 * 8)]
		ldp	    x6,  x7, [sp, #(IFRAME_x + 6 * 8)]
		ldp	    x8,  x9, [sp, #(IFRAME_x + 8 * 8)]
		ldp	   x10, x11, [sp, #(IFRAME_x + 10 * 8)]
		ldp	   x12, x13, [sp, #(IFRAME_x + 12 * 8)]
		ldp	   x14, x15, [sp, #(IFRAME_x + 14 * 8)]
		ldp	   x16, x17, [sp, #(IFRAME_x + 16 * 8)]
.if \el == 0
		// x18 and x19 will be restored later in EL1
		ldp x18, x19, [x19, #(IFRAME_x + 18 * 8)]
.endif
		ldp	   x20, x21, [sp, #(IFRAME_x + 20 * 8)]
		ldp	   x22, x23, [sp, #(IFRAME_x + 22 * 8)]
		ldp	   x24, x25, [sp, #(IFRAME_x + 24 * 8)]
		ldp	   x26, x27, [sp, #(IFRAME_x + 26 * 8)]
		ldp	   x28, fp,  [sp, #(IFRAME_x + 28 * 8)]

.if \el == 0
		// unwind our own stack pointer
		add sp, sp, #(IFRAME_sizeof)
.else
		// we stored original pointer to IFRAME, no need to unwind again there
		mov x19, sp
		mov sp, x18
		// finally restore remaining registers
		ldp x18, x19, [x19, #(IFRAME_x + 18 * 8)]
.endif

		eret
		dsb sy
		isb
	FUNCTION_END(exception_return_\el)
.endm

EXCEPTION_RETURN 0
EXCEPTION_RETURN 1

.macro	vector	func el
	.align 7
	EXCEPTION_ENTRY \el
	mov x0, sp
	mov x29, x0
	bl \func
	b exception_return_\el
	brk 0xfff
.endm

.macro	vempty
	.align 7
	brk	0xfff
	1: b	1b
.endm

.align 11
.globl _exception_vectors
_exception_vectors:
	vempty             /* Synchronous EL1t */
	vempty             /* IRQ EL1t */
	vempty             /* FIQ EL1t */
	vempty             /* Error EL1t */

	vector do_sync_handler 1  /* Synchronous EL1h */
	vector do_irq_handler 1   /* IRQ EL1h */
	vector do_fiq_handler 1   /* FIQ EL1h */
	vector do_error_handler 1 /* Error EL1h */

	vector do_sync_handler 0   /* Synchronous 64-bit EL0 */
	vector do_irq_handler 0    /* IRQ 64-bit EL0 */
	vector do_fiq_handler 0    /* FIQ 64-bit EL0 */
	vector do_error_handler 0  /* Error 64-bit EL0 */

	vempty             /* Synchronous 32-bit EL0 */
	vempty             /* IRQ 32-bit EL0 */
	vempty             /* FIQ 32-bit EL0 */
	vempty             /* Error 32-bit EL0 */

FUNCTION(_eret_with_iframe):
	mov x20, xzr
	mov x21, xzr
	mov x22, xzr
	mov x23, xzr
	mov x24, xzr
	mov x25, xzr
	mov x26, xzr
	mov x27, xzr
	mov x28, xzr
	mov x29, xzr
	mov sp, x0
	b exception_return_0
FUNCTION_END(_eret_with_iframe)

FUNCTION(_fp_save):
	stp q0, q1, [x0], #32
	stp q2, q3, [x0], #32
	stp q4, q5, [x0], #32
	stp q6, q7, [x0], #32
	stp q8, q9, [x0], #32
	stp q10, q11, [x0], #32
	stp q12, q13, [x0], #32
	stp q14, q15, [x0], #32
	stp q16, q17, [x0], #32
	stp q18, q19, [x0], #32
	stp q20, q21, [x0], #32
	stp q22, q23, [x0], #32
	stp q24, q25, [x0], #32
	stp q26, q27, [x0], #32
	stp q28, q29, [x0], #32
	stp q30, q31, [x0], #32
	mrs x1, FPSR
	mrs x2, FPCR
	str x1, [x0], #8
	str x2, [x0], #8

	// reset FPCR and FPSR to prevent userspace state affecting kernel
	msr FPSR, xzr
	cmp x2, xzr
	beq 1f
	msr FPCR, xzr
1:
	ret
FUNCTION_END(_fp_save)

FUNCTION(_fp_restore):
	ldp q0, q1, [x0], #32
	ldp q2, q3, [x0], #32
	ldp q4, q5, [x0], #32
	ldp q6, q7, [x0], #32
	ldp q8, q9, [x0], #32
	ldp q10, q11, [x0], #32
	ldp q12, q13, [x0], #32
	ldp q14, q15, [x0], #32
	ldp q16, q17, [x0], #32
	ldp q18, q19, [x0], #32
	ldp q20, q21, [x0], #32
	ldp q22, q23, [x0], #32
	ldp q24, q25, [x0], #32
	ldp q26, q27, [x0], #32
	ldp q28, q29, [x0], #32
	ldp q30, q31, [x0], #32

	ldr x1, [x0], #8
	msr FPSR, x1

	// avoid restoring FPCR if it hasn't changed
	ldr x2, [x0], #8
	mrs x3, FPCR
	cmp x3, x2
	beq 1f
	msr FPCR, x2
1:
	ret
FUNCTION_END(_fp_restore)

FUNCTION(_arch_context_swap):
	// save
	stp x19, x20, [x0], #16
	stp x21, x22, [x0], #16
	stp x23, x24, [x0], #16
	stp x25, x26, [x0], #16
	stp x27, x28, [x0], #16
	stp x29, x30, [x0], #16

	mov x2, sp
	mrs x3, TPIDR_EL0
	stp  x2,  x3, [x0], #16

	stp  d8,  d9, [x0], #16
	stp d10, d11, [x0], #16
	stp d12, d13, [x0], #16
	stp d14, d15, [x0], #16

	// restore
	ldp x19, x20, [x1], #16
	ldp x21, x22, [x1], #16
	ldp x23, x24, [x1], #16
	ldp x25, x26, [x1], #16
	ldp x27, x28, [x1], #16
	ldp x29, x30, [x1], #16

	ldp  x2,  x3, [x1], #16
	mov sp, x2
	msr TPIDR_EL0, x3

	ldp  d8,  d9, [x1], #16
	ldp d10, d11, [x1], #16
	ldp d12, d13, [x1], #16
	ldp d14, d15, [x1], #16

	// pass x29 as argument to thread entry function
	mov x0, x29
	ret
FUNCTION_END(_arch_context_swap)

/*!	\fn void arch_debug_call_with_fault_handler(cpu_ent* cpu,
		jmp_buf jumpBuffer, void (*function)(void*), void* parameter)

	Called by debug_call_with_fault_handler() to do the dirty work of setting
	the fault handler and calling the function. If the function causes a page
	fault, the arch_debug_call_with_fault_handler() calls longjmp() with the
	given \a jumpBuffer. Otherwise it returns normally.

	debug_call_with_fault_handler() has already saved the CPU's fault_handler
	and fault_handler_stack_pointer and will reset them later, so
	arch_debug_call_with_fault_handler() doesn't need to care about it.

	\param cpu The \c cpu_ent for the current CPU.
	\param jumpBuffer Buffer to be used for longjmp().
	\param function The function to be called.
	\param parameter The parameter to be passed to the function to be called.
*/
FUNCTION(arch_debug_call_with_fault_handler):
	adrp x4, fault
	add x4, x4, :lo12:fault
	str x4, [x0, #CPU_ENT_fault_handler]
	str x1, [x0, #CPU_ENT_fault_handler_stack_pointer]

	mov x0, x3
	br x2

fault:
	mov x0, sp
	mov x1, #1
	b longjmp
FUNCTION_END(arch_debug_call_with_fault_handler)


/* addr_t arm64_get_fp(void) */
FUNCTION(arm64_get_fp):
	mov x0, x29
	ret
FUNCTION_END(arm64_get_fp)
