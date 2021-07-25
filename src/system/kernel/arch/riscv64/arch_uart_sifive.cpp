/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */

#include <arch/riscv64/arch_uart_sifive.h>


ArchUARTSifive::ArchUARTSifive(addr_t base, int64 clock)
	:
	DebugUART(base, clock)
{
}


ArchUARTSifive::~ArchUARTSifive()
{
}


void
ArchUARTSifive::InitEarly()
{
	//Regs()->ie = 0;
	//Enable();
}


void
ArchUARTSifive::Init()
{
}


void
ArchUARTSifive::InitPort(uint32 baud)
{
	uint64 quotient = (Clock() + baud - 1) / baud;

	if (quotient == 0)
		Regs()->div = 0;
	else
		Regs()->div = (uint32)(quotient - 1);
}


void
ArchUARTSifive::Enable()
{
	//Regs()->txctrl.enable = 1;
	//Regs()->rxctrl.enable = 1;
	DebugUART::Enable();
}


void
ArchUARTSifive::Disable()
{
	//Regs()->txctrl.enable = 0;
	//Regs()->rxctrl.enable = 0;
	DebugUART::Disable();
}


int
ArchUARTSifive::PutChar(char ch)
{
	while (Regs()->txdata.isFull) {}
	Regs()->txdata.val = ch;
	return 0;
}


int
ArchUARTSifive::GetChar(bool wait)
{
	if (wait) {
		uint32 val;
		while ((val = GetChar(false)) < 0) {}
		return val;
	}
	UARTSifiveRegs::Rxdata data = {.val = Regs()->rxdata.val};
	if (data.isEmpty)
		return -1;

	return data.data;
}


void
ArchUARTSifive::FlushTx()
{
}


void
ArchUARTSifive::FlushRx()
{
}


void
ArchUARTSifive::Barrier()
{
	asm volatile ("" : : : "memory");
}
