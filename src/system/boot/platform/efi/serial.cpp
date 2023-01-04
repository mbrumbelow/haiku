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
#if defined(__i386__) || defined(__x86_64__)
#	include <arch/x86/arch_uart_x86legacy.h>
#endif
#include <boot/stage2.h>
#include <boot/stdio.h>

#include <string.h>


static efi_guid sSerialIOProtocolGUID = EFI_SERIAL_IO_PROTOCOL_GUID;
static const uint32 kSerialBaudRate = 115200;

static efi_serial_io_protocol *sSerial = NULL;
static bool sSerialEnabled = false;


DebugUART* gUART = NULL;


static void
serial_putc(char ch)
{
	if (!sSerialEnabled)
		return;

	// First we prefer any UARTs we know about...
	if (gUART != NULL) {
		gUART->PutChar(ch);
		return;
	}

	// Then we use EFI serial output if available
	if (sSerial != NULL) {
		size_t bufSize = 1;
		sSerial->Write(sSerial, &bufSize, &ch);
		return;
	}

#ifdef DEBUG
	// To aid in early bring-up on EFI platforms, where the
	// serial_io protocol isn't working/available.
	char16_t ucsBuffer[2];
	ucsBuffer[0] = ch;
	ucsBuffer[1] = 0;
	kSystemTable->ConOut->OutputString(kSystemTable->ConOut, ucsBuffer);
	return;
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
}


extern "C" void
serial_init(void)
{

#if defined(__i386__) || defined(__x86_64__)
	// TODO: Move to arch_serial_init or something?
	if (gUART == NULL)
		gUART = arch_get_uart_x86legacy(0x3f8, kSerialBaudRate);
#endif

#if defined(__i386__) || defined(__x86_64__)
	// On x86, we tell the kernel to use the legacy serial base port
	// This likely needs reworked to better use gUART
	memset(gKernelArgs.platform_args.serial_base_ports, 0,
		sizeof(uint16) * MAX_SERIAL_PORTS);
	gKernelArgs.platform_args.serial_base_ports[0] = 0x3f8;
#endif

	// If we have a UART, use it instead of EFI Serial services
	if (gUART != NULL)
		return;

	// Check for EFI Serial
	efi_status status = kSystemTable->BootServices->LocateProtocol(
		&sSerialIOProtocolGUID, NULL, (void**)&sSerial);

	if (status != EFI_SUCCESS || sSerial == NULL) {
		sSerial = NULL;
		return;
	}

	// Setup serial, 0, 0 = Default Receive FIFO queue and default timeout
	status = sSerial->SetAttributes(sSerial, kSerialBaudRate, 0, 0, NoParity, 8,
		OneStopBit);

	if (status != EFI_SUCCESS) {
		sSerial = NULL;
		return;
	}
}


extern "C" void
serial_uninit(void)
{
	// Disconnecting from EFI bios services is important as we leave the bootloader
	sSerial = NULL;
}
