/*
 * Copyright 2023, Keshav Gupta, iamkeshav06@gmail.com
 * Distributed under the terms of the MIT License.
 */

#include <OS.h>
#include "syscalls.h"

void __arch_clear_cache(void *address, size_t length, uint32 flags)
{
	_kern_clear_caches(address, length, flags);
}