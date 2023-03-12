/*
 * Copyright 2012-2023 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */
#ifndef _KERNEL_ARCH_DEBUG_UART_EFI_H
#define _KERNEL_ARCH_DEBUG_UART_EFI_H


#include <arch/generic/debug_uart.h>
#include <efi/protocol/serial-io.h>
#include <sys/types.h>

#include <SupportDefs.h>


// No UART_KIND_EFI. I should never be passed into kernel as EFI BIOS
// services not available in kernel after EFI exit


class DebugUARTEFI : public DebugUART {
public:
							DebugUARTEFI(addr_t base, int64 clock);
							~DebugUARTEFI();

			void			InitEarly();
			void			Init();
			void			InitPort(uint32 baud);

			int				PutChar(char c);
			int				GetChar(bool wait);

			void			FlushTx();
			void			FlushRx();
private:
			efi_serial_io_protocol*	fEFISerialIO;
};


DebugUARTEFI* debug_get_uart_efi(addr_t base, int64 clock);


#endif /* _KERNEL_ARCH_DEBUG_UART_EFI_H */
