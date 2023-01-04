/*
 * Copyright 2011-2021 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef __DEV_UART_X86_LEGACY_H
#define __DEV_UART_X86_LEGACY_H


#include <sys/types.h>

#include <SupportDefs.h>

#include <arch/generic/debug_uart.h>


#define UART_KIND_X86LEGACY "x86 Legacy"


class ArchUARTx86Legacy : public DebugUART {
public:
							ArchUARTx86Legacy(addr_t base, int64 clock);
							~ArchUARTx86Legacy();

			void			InitEarly();
			void			InitPort(uint32 baud);

			void			Enable();
			void			Disable();

			int				PutChar(char c);
			int				GetChar(bool wait);

			void			FlushTx();
			void			FlushRx();

private:
	virtual void			Out8(int reg, uint8 value);
	virtual uint8			In8(int reg);
};


ArchUARTx86Legacy *arch_get_uart_x86legacy(addr_t base, int64 clock);


#endif
