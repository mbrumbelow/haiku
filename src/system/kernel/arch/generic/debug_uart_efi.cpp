/*
 * Copyright 2016-2023, Haiku, Inc., All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <arch/generic/debug_uart_efi.h>
#include <debug.h>
#include <new>


DebugUARTEFI::DebugUARTEFI(addr_t base, int64 clock)
	:
	DebugUART(base, clock)
{
	fEFISerialIO = (efi_serial_io_protocol*)base;
}


DebugUARTEFI::~DebugUARTEFI()
{
	fEFISerialIO = NULL;
}


void
DebugUARTEFI::InitPort(uint32 baud)
{
	Disable();

	if (fEFISerialIO != NULL) {
		// Setup serial, 0, 0 = Default Receive FIFO queue and default timeout
		efi_status status = fEFISerialIO->SetAttributes(fEFISerialIO, baud, 0, 0,
			NoParity, 8, OneStopBit);

		if (status != EFI_SUCCESS)
			fEFISerialIO = NULL;
		else
		Enable();
	}
}


void
DebugUARTEFI::InitEarly()
{
}


void
DebugUARTEFI::Init()
{
}


int
DebugUARTEFI::PutChar(char c)
{
	if (fEFISerialIO != NULL) {
		size_t bufSize = 1;
		fEFISerialIO->Write(fEFISerialIO, &bufSize, &c);
		return 0;
	}
	return -1;
}


/* returns -1 if no data available */
int
DebugUARTEFI::GetChar(bool wait)
{
	// TODO
	return -1;
}


void
DebugUARTEFI::FlushTx()
{
}


void
DebugUARTEFI::FlushRx()
{
}


DebugUARTEFI*
arch_get_uart_EFI(addr_t base, int64 clock)
{
	static char buffer[sizeof(DebugUARTEFI)];
	DebugUARTEFI* uart = new(buffer) DebugUARTEFI(base, clock);
	return uart;
}
