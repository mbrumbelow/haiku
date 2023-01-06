/*
 * Copyright 2006-2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <arch/generic/debug_uart.h>


void
DebugUART::Out8(int reg, uint8 value)
{
	addr_t address = (Base() + (reg << RegShift()));
#if defined(__i386__) || defined(__x86_64__)
	// outb for access to IO space.
	if (address <= 0xFFFF) {
		__asm__ volatile ("outb %%al,%%dx" : : "a" (value), "d" (address));
		return;
	}
#endif
	*((uint8 *)address) = value;
}


uint8
DebugUART::In8(int reg)
{
	addr_t address = (Base() + (reg << RegShift()));
#if defined(__i386__) || defined(__x86_64__)
	// inb for access to IO space.
	if (address <= 0xFFFF) {
		uint8 _v;
		__asm__ volatile ("inb %%dx,%%al" : "=a" (_v) : "d" (address));
		return _v;
	}
#endif
	return *((uint8 *)address);
}


void
DebugUART::Barrier()
{
	// Simple memory barriers
#if defined(__POWERPC__)
	asm volatile("eieio; sync");
#elif defined(__ARM__) || defined(__aarch64__)
	asm volatile ("" : : : "memory");
#endif
}
