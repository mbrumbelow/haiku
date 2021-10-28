/*
 * Copyright 2019-2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include <int.h>

#include <platform/openfirmware/openfirmware.h>


status_t
arch_int_init(kernel_args *args)
{
	// Take over fault handling from openfirmware
	extern uint32 gTrapHandler;
	of_call_client_function("SUNW,set-trap-table", 1, 0, &gTrapHandler);
	return B_OK;
}


status_t
arch_int_init_post_vm(kernel_args *args)
{
	return B_OK;
}


status_t
arch_int_init_post_device_manager(struct kernel_args *args)
{
	return B_OK;
}


status_t
arch_int_init_io(kernel_args* args)
{
	return B_OK;
}


void
arch_int_enable_io_interrupt(int irq)
{
}


void
arch_int_disable_io_interrupt(int irq)
{
}


void
arch_int_assign_to_cpu(int32 irq, int32 cpu)
{
}
