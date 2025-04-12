/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * Copyright 2010, Andreas Faerber <andreas.faerber@web.de>
 * Copyright 2002, Adrien Destugues <pulkomandy@pulkomandy.tk>
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <new>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <OS.h>

#include <boot/net/DHCP.h>
#include <boot/net/Ethernet.h>
#include <boot/net/IP.h>
#include <boot/net/NetStack.h>
#include <boot/platform.h>
#include <platform/openfirmware/devices.h>
#include <platform/openfirmware/openfirmware.h>


//#define TRACE_NETWORK
#ifdef TRACE_NETWORK
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


class OFEthernetInterface : public EthernetInterface {
public:
						OFEthernetInterface();
	virtual				~OFEthernetInterface();

			status_t	Init(const char *device, const char *parameters);

	virtual mac_addr_t	MACAddress() const;

	virtual	void *		AllocateSendReceiveBuffer(size_t size);
	virtual	void		FreeSendReceiveBuffer(void *buffer);
	void				acquireIPoverDHCP();

	virtual ssize_t		Send(const void *buffer, size_t size);
	virtual ssize_t		Receive(void *buffer, size_t size);

private:
			status_t	FindMACAddress();

private:
			intptr_t	fHandle;
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
					printf("\n");
				printf("%03x: ", i);
			} else
				printf(" ");
		}

		printf("%02x", data[i]);
	}
	printf("\n");
}

#else	// !TRACE_NETWORK

#define hex_dump(data, length)

#endif	// !TRACE_NETWORK


// #pragma mark -


OFEthernetInterface::OFEthernetInterface()
	:
	EthernetInterface(),
	fHandle(OF_FAILED),
	fMACAddress(kNoMACAddress)
{
}


OFEthernetInterface::~OFEthernetInterface()
{
	if (fHandle != OF_FAILED)
		of_close(fHandle);
}


status_t
OFEthernetInterface::FindMACAddress()
{
	intptr_t package = of_instance_to_package(fHandle);

	// get MAC address
	int bytesRead = of_getprop(package, "local-mac-address", &fMACAddress,
		sizeof(fMACAddress));
	if (bytesRead == (int)sizeof(fMACAddress))
		return B_OK;

	// Failed to get the MAC address of the network device. The system may
	// have a global standard MAC address.
	bytesRead = of_getprop(gChosen, "mac-address", &fMACAddress,
		sizeof(fMACAddress));
	if (bytesRead == (int)sizeof(fMACAddress)) {
		return B_OK;
	}

	// On Sun machines, there is a global word 'mac-address' which returns
	// the size and a pointer to the MAC address
	size_t size;
	void* ptr;
	if (of_interpret("mac-address", 0, 2, &size, &ptr) != OF_FAILED) {
		if (size == sizeof(fMACAddress)) {
			memcpy(&fMACAddress, ptr, size);
			return B_OK;
		}
	}

	return B_ERROR;
}


status_t
OFEthernetInterface::Init(const char *device, const char *parameters)
{
	dprintf("Initializing ethernet device\n");
	if (!device)
		return B_BAD_VALUE;

	// open device
	fHandle = of_open(device);
	if (fHandle == OF_FAILED) {
		dprintf("opening ethernet device failed\n");
		return B_ERROR;
	}

	if (FindMACAddress() != B_OK) {
		dprintf("Failed to get MAC address\n");
		return B_ERROR;
	}

	// get IP address

	// Note: This is a non-standardized way. On my Mac mini the response of the
	// DHCP server is stored as property of /chosen. We try to get it and use
	// the IP address we find in there.
	// TODO Sun machines may use bootp-response instead?
	struct {
		uint8	irrelevant[16];
		uint32	ip_address;
		// ...
	} dhcpResponse;
	int bytesRead = of_getprop(gChosen, "dhcp-response", &dhcpResponse,
		sizeof(dhcpResponse));
	if (bytesRead != OF_FAILED && bytesRead == (int)sizeof(dhcpResponse)) {
		SetIPAddress(ntohl(dhcpResponse.ip_address));
		return B_OK;
	}

	// try to read default-client-ip setting
	char defaultClientIP[16];
	intptr_t package = of_finddevice("/options");
	bytesRead = of_getprop(package, "default-client-ip",
		defaultClientIP, sizeof(defaultClientIP) - 1);
	if (bytesRead != OF_FAILED && bytesRead > 1) {
		defaultClientIP[bytesRead] = '\0';
		ip_addr_t address = ip_parse_address(defaultClientIP);
		SetIPAddress(address);
		return B_OK;
	}

	// if we get here, we have to do dhcp ourselves. However, the DHCP
	// client cannot be run until the Ethernet device is registered.
	SetIPAddress(INADDR_ANY);

	return B_OK;
}

void
OFEthernetInterface::acquireIPoverDHCP()
{
	DHCPClient dhcpClient(this);
	if (dhcpClient.run() != B_OK) {
		dprintf("Could not aquire IP address!\n");
	} else {
		dprintf("Aquired the IP Address %X\n", dhcpClient.IPAddress());
		SetIPAddress(dhcpClient.IPAddress());
	}
}


mac_addr_t
OFEthernetInterface::MACAddress() const
{
	return fMACAddress;
}


void *
OFEthernetInterface::AllocateSendReceiveBuffer(size_t size)
{
	void *dmaMemory = NULL;

	if (of_call_method(fHandle, "dma-alloc", 1, 1, size, &dmaMemory)
			!= OF_FAILED) {
		return dmaMemory;
	}

	// The dma-alloc method could be on the parent node (PCI bus, for example),
	// rather than the device itself
	intptr_t parentPackage = of_parent(of_instance_to_package(fHandle));

	// FIXME surely there's a way to create an instance without going through
	// the path?
	char path[256];
	of_package_to_path(parentPackage, path, sizeof(path));
	intptr_t parentInstance = of_open(path);

	if (of_call_method(parentInstance, "dma-alloc", 1, 1, size, &dmaMemory)
			!= OF_FAILED) {
		of_close(parentInstance);
		return dmaMemory;
	}

	of_close(parentInstance);

	return NULL;
}


void
OFEthernetInterface::FreeSendReceiveBuffer(void *buffer)
{
	if (buffer)
		of_call_method(fHandle, "dma-free", 1, 0, buffer);
}


ssize_t
OFEthernetInterface::Send(const void *buffer, size_t size)
{
	TRACE(("OFEthernetInterface::Send(%p, %lu)\n", buffer, size));

	if (!buffer)
		return B_BAD_VALUE;

	hex_dump(buffer, size);

	int result = of_write(fHandle, buffer, size);
	return (result == OF_FAILED ? B_ERROR : result);
}


ssize_t
OFEthernetInterface::Receive(void *buffer, size_t size)
{
	if (!buffer)
		return B_BAD_VALUE;

	int result = of_read(fHandle, buffer, size);

	if (result != OF_FAILED && result >= 0) {
		TRACE(("OFEthernetInterface::Receive(%p, %lu): received %d bytes\n",
			buffer, size, result));
		hex_dump(buffer, result);
	}

	return (result == OF_FAILED ? B_ERROR : result);
}


// #pragma mark -


status_t
platform_net_stack_init()
{
	intptr_t cookie = 0;
	char path[256];
	status_t status;

	// get all network devices
	while ((status = of_get_next_device(&cookie, 0, "network", path, sizeof(path))) == B_OK) {

		dprintf("Found network device %s\n", path);

		// get device node
		intptr_t node = of_finddevice(path);
		if (node == OF_FAILED) {
			dprintf("\tCould'nt find the node of the network device!\n");
			continue;
		}

		// get device type
		char type[16];
		if (of_getprop(node, "device_type", type, sizeof(type)) == OF_FAILED
			|| strcmp("network", type) != 0) {
			dprintf("\tdevice is not a network device!?\n");
			continue;
		}

		// create an EthernetInterface object for the device
		OFEthernetInterface* interface = new(nothrow) OFEthernetInterface;
		if (!interface) {
			dprintf("\tInsufficient memory to create device!\n");
			return B_NO_MEMORY;
		}

		// initialize the ethernet interface
		status_t error = interface->Init(path, nullptr);
		if (error != B_OK) {
			delete interface;
			dprintf("\tCould'nt initialize the Ethernet Interface!\n");
		} else {
			// add it to the net stack
			error = NetStack::Default()->AddEthernetInterface(interface);
			if (error != B_OK) {
				dprintf("\tCouldn't add Ethernet Interface to the stack!\n");
				delete interface;
			}

			// check if DHCP has to be run
			if (interface->IPAddress() == INADDR_ANY)
				interface->acquireIPoverDHCP();
		}
	}

	return B_OK;
}
