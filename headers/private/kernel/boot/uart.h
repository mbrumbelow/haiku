/*
 * Copyright 2009-2019 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_BOOT_UART_H
#define KERNEL_BOOT_UART_H


typedef struct {
	char kind[32];
	addr_range regs;
	uint32 irq;
	int64 clock;
} __attribute__((packed)) uart_info;


#endif
