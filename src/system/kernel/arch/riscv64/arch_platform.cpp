/* Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk.
 * Distributed under the terms of the MIT License.
 */


#include <arch/platform.h>


status_t
arch_platform_init(struct kernel_args *kernelArgs)
{
	return B_OK;
}


status_t
arch_platform_init_post_vm(struct kernel_args *kernelArgs)
{
	return B_OK;
}


status_t
arch_platform_init_post_thread(struct kernel_args *kernelArgs)
{
	return B_OK;
}


status_t
arch_platform_convert_args(struct kernel_args *args, bool* upgraded)
{
	if (args->version == KERNEL_ARGS_VERSION_V1
			&& args->kernel_args_size == sizeof(kernel_args)) {
		args->version = KERNEL_ARGS_VERSION_V2;
		if (upgraded != NULL)
			*upgraded = true;
	}
	return B_OK;
}
