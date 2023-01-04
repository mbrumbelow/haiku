/*
 * Copyright 2011-2021 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 */


#include <debug.h>
#include <arch/generic/debug_uart.h>
#include <arch/x86/arch_uart_x86legacy.h>
#include <new>


enum {
	SERIAL_TRANSMIT_BUFFER		= 0,
	SERIAL_RECEIVE_BUFFER		= 0,
	SERIAL_DIVISOR_LATCH_LOW	= 0,
	SERIAL_DIVISOR_LATCH_HIGH	= 1,
	SERIAL_FIFO_CONTROL			= 2,
	SERIAL_LINE_CONTROL			= 3,
	SERIAL_MODEM_CONTROL		= 4,
	SERIAL_LINE_STATUS			= 5,
	SERIAL_MODEM_STATUS			= 6,
};


ArchUARTx86Legacy::ArchUARTx86Legacy(addr_t base, int64 clock)
	:
	DebugUART(base, clock)
{
}


ArchUARTx86Legacy::~ArchUARTx86Legacy()
{
}


void
ArchUARTx86Legacy::InitPort(uint32 baud)
{
	// Disable UART
	Disable();

	//memset(gKernelArgs.platform_args.serial_base_ports, 0,
	//	sizeof(uint16) * MAX_SERIAL_PORTS);
	//gKernelArgs.platform_args.serial_base_ports[0] = sSerialBasePort;

	uint16 divisor = uint16(115200 / baud);

	Out8(0x80, SERIAL_LINE_CONTROL);
		// set divisor latch access bit
	Out8(divisor & 0xf, SERIAL_DIVISOR_LATCH_LOW);
	Out8(divisor >> 8, SERIAL_DIVISOR_LATCH_HIGH);
	Out8(3, SERIAL_LINE_CONTROL);
		// 8N1

	// Enable UART
	Enable();
}


void
ArchUARTx86Legacy::InitEarly()
{
	// Perform special hardware UART configuration
}


void
ArchUARTx86Legacy::Enable()
{
	DebugUART::Enable();
}


void
ArchUARTx86Legacy::Disable()
{
	DebugUART::Disable();
}


void
ArchUARTx86Legacy::Out8(int reg, uint8 value)
{
	*((uint8 *)Base() + reg) = value;
}


uint8
ArchUARTx86Legacy::In8(int reg)
{
	return *((uint8 *)Base() + reg);
}


int
ArchUARTx86Legacy::PutChar(char c)
{
	if (Enabled() == true) {
		while ((In8(SERIAL_LINE_STATUS) & 0x20) == 0)
			asm volatile ("pause;");
		Out8(c, SERIAL_TRANSMIT_BUFFER);
		return 0;
	}

	return -1;
}


int
ArchUARTx86Legacy::GetChar(bool wait)
{
	// TODO
	return -1;
}


void
ArchUARTx86Legacy::FlushTx()
{
	// TODO
}


void
ArchUARTx86Legacy::FlushRx()
{
	// TODO
}


ArchUARTx86Legacy*
arch_get_uart_x86legacy(addr_t base, int64 clock)
{
	static char buffer[sizeof(ArchUARTx86Legacy)];
	ArchUARTx86Legacy *uart = new(buffer) ArchUARTx86Legacy(base, clock);
	return uart;
}
