/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2013-2014, Fredrik Holmqvist, fredrik.holmqvist@gmail.com.
 * Copyright 2016, Jessica Hamilton, jessica.l.hamilton@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include "efi_platform.h"
#include <efi/protocol/serial-io.h>
#include "serial.h"

#include <boot/platform.h>
#include <arch/cpu.h>
#include <arch/generic/debug_uart.h>
#include <arch/generic/debug_uart_8250.h>
#include <boot/stage2.h>
#include <boot/stdio.h>

#include <string.h>

#include "debug_uart_efi.h"


static efi_guid sSerialIOProtocolGUID = EFI_SERIAL_IO_PROTOCOL_GUID;
static const uint32 kSerialBaudRate = 115200;

static bool sSerialEnabled = false;
static bool sEFIAvailable = true;


DebugUART* gUART = NULL;
DebugUART* gHardwareUART = NULL;


static void
serial_putc(char ch)
{
	if (!sSerialEnabled)
		return;

	if (gUART != NULL)
		gUART->PutChar(ch);

	#ifdef DEBUG
	// If DEBUG build, also use stdout to make sure we get logs
	if (sEFIAvailable) {
		char16_t ucsBuffer[2];
		ucsBuffer[0] = ch;
		ucsBuffer[1] = 0;
		kSystemTable->ConOut->OutputString(kSystemTable->ConOut, ucsBuffer);
		return;
	}
	#endif
}


extern "C" void
serial_puts(const char* string, size_t size)
{
	if (!sSerialEnabled)
		return;

	while (size-- != 0) {
		char ch = string[0];

		if (ch == '\n') {
			serial_putc('\r');
			serial_putc('\n');
		} else if (ch != '\r')
			serial_putc(ch);

		string++;
	}
}


extern "C" void
serial_disable(void)
{
	sSerialEnabled = false;
}


extern "C" void
serial_enable(void)
{
	sSerialEnabled = true;
	if (gUART != NULL)
		gUART->InitPort(kSerialBaudRate);
}


extern "C" void
serial_init(void)
{
	// If EFI BIOS Services are available, always prefer them
	if (gUART == NULL && sEFIAvailable) {
		efi_serial_io_protocol *serialIO = NULL;

		// Check for EFI Serial
		efi_status status = kSystemTable->BootServices->LocateProtocol(
			&sSerialIOProtocolGUID, NULL, (void**)&serialIO);

		if (status == EFI_SUCCESS)
			gUART = debug_get_uart_efi((addr_t)serialIO, 0);
	}

#if defined(__i386__) || defined(__x86_64__)
	// On x86, we can try to setup COM1 as a gUART too
	// while this serial port may not physically exist,
	// the location is fixed on the x86 arch.
	// TODO: We could also try to pull from acpi?
	if (gUART == NULL) {
		gUART = arch_get_uart_8250(0x3f8, 1843200);

		// TODO: convert over to exclusively arch_args.uart?
		memset(gKernelArgs.platform_args.serial_base_ports, 0,
			sizeof(uint16) * MAX_SERIAL_PORTS);
		gKernelArgs.platform_args.serial_base_ports[0] = 0x3f8;
	}
#endif

	if (gUART != NULL)
		gUART->InitEarly();
}


extern "C" void
serial_kernel_handoff(void)
{
	// The console was provided by boot services, disable it ASAP
	stdout = NULL;
	stderr = NULL;
	sEFIAvailable = false;

	// Disconnect from EFI serial_io services is important as we leave the bootloader
	if (gUART != NULL)
		gUART = NULL;

	// Assign any located hardware UARTS to our UART until we're in the kernel
	if (gHardwareUART != NULL)
		gUART = gHardwareUART;
}
