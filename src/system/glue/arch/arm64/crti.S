/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

.section .init, "ax", @progbits
.p2align 16
.globl _init
.type _init, %function
_init:
	sub		sp, sp, #16
	str		lr, [sp]

.section .fini, "ax", @progbits
.p2align 16
.globl _fini
.type _fini, %function
_fini:
	sub		sp, sp, #16
	str		lr, [sp]
