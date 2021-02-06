/*
 * Copyright 2020-2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <asm_defs.h>

    .text

/*  status_t arch_enter_kernel(uint64 sv39, struct kernel_args *kernelArgs,
        addr_t kernelEntry, addr_t kernelStackTop);

    a0  - sv39
    a1  - kernelArgs
    a2  - kernelEntry
    a3  - kernelStackTop
*/
FUNCTION(arch_enter_kernel):
	// set the kernel stack
	mv	sp,a3

	// TODO: Use provided sv39 page table

	// Setup kernel args
	mv	a0,a1	// kernelArgs
	mv	a4,a2
	li	a1,0	// currentCPU=0

	// call the kernel
	jr	a4

	// return
	li	a0,-1  // B_ERROR
	ret
FUNCTION_END(arch_enter_kernel)
