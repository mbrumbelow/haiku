/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#define FUNCTION(x) .global x; .type x,@function; x

/*	status_t arch_start_kernel(struct kernel_args *kernelArgs,
		addr_t kernelEntry, addr_t kernelStackTop);

	o0	- kernelArgs
	o1	- kernelEntry
	o2	- kernelStackTop
*/
FUNCTION(arch_start_kernel):

	// Save the old stack
	mov %sp, %g5

	// Set the stack
	sub %o2, 2047, %sp // apply stack bias

	mov %o1, %o2 // Move the kernelEntry as we need o1 for something else

	// o0 is already the kernel args, leave it there

	// Set up "current cpu" argument to kernel _start
	mov %g0, %o1

	// Jump into the kernel, do not store return address
	jmpl %o2, %o7
	nop // branch delay slot...

	// We should never get here, but in case the kernel return, attempt to
	// exit cleanly (unlikely to work if the kernel has used g5 for something
	// else... we should rather use a proper save/restore and keep things in
	// our local registers)

	// Restore the bootloader stack
	mov %g5, %sp

	// Exit the function
	retl
	nop // branch delay slot...
