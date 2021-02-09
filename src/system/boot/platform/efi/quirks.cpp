/*
 * Copyright 2015, Bruno Bierbaumer. All rights reserved.
 * Copyright 2019-2020, Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License
 */


#include <efi/protocol/apple-setos.h>
#include <efi/protocol/console-control.h>
#include <KernelExport.h>

#include "efi_platform.h"


#define APPLE_FAKE_OS_VENDOR "Apple Inc."
#define APPLE_FAKE_OS_VERSION "Mac OS X 10.9"


// Unofficial control of UEFI text / graphics modes
// Implemented wildly different under different EFI
// implementations.
static status_t
quirks_console_control(bool graphics)
{
	efi_guid consoleControlProtocolGUID = EFI_CONSOLE_CONTROL_PROTOCOL_GUID;
	efi_console_control_protocol* consoleControl = NULL;

	efi_status status = kSystemTable->BootServices->LocateProtocol(
		&consoleControlProtocolGUID, NULL, (void**)&consoleControl);

	// Some EFI implementations boot up in an EFI graphics mode (Apple)
	// If this protocol doesn't exist, we can assume we're already in text mode.
	if (status != EFI_SUCCESS || consoleControl == NULL)
		return B_ERROR;

	dprintf("%s: Located EFI console control. Setting EFI %s mode...\n",
		__func__, graphics ? "graphics" : "text");

	if (graphics) {
		status = consoleControl->SetMode(consoleControl,
			EfiConsoleControlScreenGraphics);
	} else {
		status = consoleControl->SetMode(consoleControl,
			EfiConsoleControlScreenText);
	}

	if (status != EFI_SUCCESS) {
		dprintf("%s: EFI console control failed.\n", __func__);
		return B_ERROR;
	}

	dprintf("%s: EFI console control success.\n", __func__);
	return B_OK;
}


// Apple Hardware configures hardware differently depending on
// the operating system being booted. Examples include disabling
// and powering down the internal GPU on some device models.
static status_t
quirks_fake_apple(void)
{
	efi_guid appleSetOSProtocolGUID = EFI_APPLE_SET_OS_GUID;
	efi_apple_set_os_protocol* set_os = NULL;

	efi_status status = kSystemTable->BootServices->LocateProtocol(
		&appleSetOSProtocolGUID, NULL, (void**)&set_os);

	// If not relevant, we will exit here (the protocol doesn't exist)
	if (status != EFI_SUCCESS || set_os == NULL) {
		return B_ERROR;
	}

	dprintf("%s: Located Apple set_os protocol, applying quirks...\n",
		__func__);

	if (set_os->Revision != 0) {
		status = set_os->SetOSVersion((char*)APPLE_FAKE_OS_VERSION);
		if (status != EFI_SUCCESS) {
			dprintf("%s: unable to set os version!\n", __func__);
			return B_ERROR;
		}
	}

	status = set_os->SetOSVendor((char*)APPLE_FAKE_OS_VENDOR);
	if (status != EFI_SUCCESS) {
		dprintf("%s: unable to set os version!\n", __func__);
		return B_ERROR;
	}

	return B_OK;
}


void
quirks_init(void)
{
	if (quirks_fake_apple() == B_OK) {
		// If Apple, use unofficial console_control to fix video
		quirks_console_control(false);
	}
}
