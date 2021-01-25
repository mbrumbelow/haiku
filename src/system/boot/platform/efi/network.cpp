/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * Copyright 2010, Andreas Faerber <andreas.faerber@web.de>
 * Copyright 2002, Adrien Destugues <pulkomandy@pulkomandy.tk>
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <new>

#include <stdlib.h>
#include <string.h>

#include <OS.h>

#include <boot/platform.h>
#include <boot/net/Ethernet.h>
#include <boot/net/IP.h>
#include <boot/net/NetStack.h>

#include "efi_platform.h"
#include <efi/protocol/simple-network.h>


static efi_guid sNetworkProtocolGUID = EFI_SIMPLE_NETWORK_PROTOCOL_GUID;


#define TRACE_NETWORK
#ifdef TRACE_NETWORK
#	define TRACE(x...) dprintf("efi/network: " x)
#	define CALLED(x...) dprintf("efi/network: CALLED %s", __func__)
#else
#	define TRACE(x...) ;
#	define CALLED(x...) ;
#endif

#define TRACE_ALWAYS(x...) dprintf("efi/network: " x)


class EFIEthernetInterface : public EthernetInterface {
public:
						EFIEthernetInterface();
	virtual				~EFIEthernetInterface();

			status_t	Init();

	virtual mac_addr_t	MACAddress() const;

	virtual	void *		AllocateSendReceiveBuffer(size_t size);
	virtual	void		FreeSendReceiveBuffer(void *buffer);

	virtual ssize_t		Send(const void *buffer, size_t size);
	virtual ssize_t		Receive(void *buffer, size_t size);

private:
			status_t	FindMACAddress();

private:
			efi_simple_network_protocol* fNetwork;
			mac_addr_t	fMACAddress;
};


#ifdef TRACE_NETWORK

static void
hex_dump(const void *_data, int length)
{
	uint8 *data = (uint8*)_data;
	for (int i = 0; i < length; i++) {
		if (i % 4 == 0) {
			if (i % 32 == 0) {
				if (i != 0)
					dprintf("\n");
				dprintf("%03x: ", i);
			} else
				dprintf(" ");
		}

		dprintf("%02x", data[i]);
	}
	dprintf("\n");
}

#else	// !TRACE_NETWORK

#define hex_dump(data, length)

#endif	// !TRACE_NETWORK


// #pragma mark -


EFIEthernetInterface::EFIEthernetInterface()
	:
	EthernetInterface(),
	fNetwork(NULL),
	fMACAddress(kNoMACAddress)
{
}


EFIEthernetInterface::~EFIEthernetInterface()
{
}


status_t
EFIEthernetInterface::FindMACAddress()
{
	if (fNetwork == NULL)
		return B_ERROR;

	memcpy(&fMACAddress, &fNetwork->Mode->PermanentAddress,
		sizeof(fMACAddress));

	return B_OK;
}


status_t
EFIEthernetInterface::Init()
{
	CALLED();

	efi_status status = kSystemTable->BootServices->LocateProtocol(
		&sNetworkProtocolGUID, NULL, (void**)&fNetwork);

	if (status != EFI_SUCCESS || fNetwork == NULL) {
		fNetwork = NULL;
		TRACE_ALWAYS("EFI reports network unavaiable)\n");
		return B_ERROR;
        }
	TRACE_ALWAYS("EFI reports network avaiable)\n");

	status = fNetwork->Initialize(fNetwork, 0, 0);

	if (status != EFI_SUCCESS) {
		TRACE_ALWAYS("error initializing network\n");
		return B_ERROR;
	}

	if (FindMACAddress() != B_OK) {
		TRACE_ALWAYS("failed to get MAC address\n");
		return B_ERROR;
	}

	// TODO: get IP address. Call SetIPAddress for NetStack

	return B_ERROR;
}


mac_addr_t
EFIEthernetInterface::MACAddress() const
{
	return fMACAddress;
}


void *
EFIEthernetInterface::AllocateSendReceiveBuffer(size_t size)
{
	// TODO: EFI PageAllocate?
	return NULL;
}


void
EFIEthernetInterface::FreeSendReceiveBuffer(void *buffer)
{
	// TODO: anything to free?
}


ssize_t
EFIEthernetInterface::Send(const void *buffer, size_t size)
{
	TRACE("EFIEthernetInterface::Send(%p, %lu)\n", buffer, size);

	if (!buffer)
		return B_BAD_VALUE;

	hex_dump(buffer, size);

	// TODO: Send

	return B_ERROR;
}


ssize_t
EFIEthernetInterface::Receive(void *buffer, size_t size)
{
	if (!buffer)
		return B_BAD_VALUE;

	// TODO: Receive

	return B_ERROR;
}


// #pragma mark -


status_t
platform_net_stack_init()
{
	// create an EthernetInterface object for the device
	EFIEthernetInterface *interface = new(nothrow) EFIEthernetInterface;
	if (!interface)
		return B_NO_MEMORY;

	status_t error = interface->Init();
	if (error != B_OK) {
		delete interface;
		return error;
	}

	// add it to the net stack
	error = NetStack::Default()->AddEthernetInterface(interface);
	if (error != B_OK) {
		delete interface;
		return error;
	}

	return B_OK;
}
