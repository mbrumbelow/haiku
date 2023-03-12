/*
 * Copyright 2012-2023 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */
#ifndef _KERNEL_ARCH_DEBUG_UART_EFI_H
#define _KERNEL_ARCH_DEBUG_UART_EFI_H


#include <efi/protocol/serial-io.h>
#include <sys/types.h>

#include <SupportDefs.h>

#include "debug_uart.h"

#define UART_KIND_EFI "EFI"


static efi_guid sSerialIOProtocolGUID = EFI_SERIAL_IO_PROTOCOL_GUID;

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


DebugUARTEFI* arch_get_uart_EFI(addr_t base, int64 clock);


#endif /* _KERNEL_ARCH_DEBUG_UART_EFI_H */
