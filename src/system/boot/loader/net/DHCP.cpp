/*
 * Copyright 2025, Maite Gamper <mail@zeldakatze.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <new>

#include <endian.h>
#include <stdio.h>

#include <boot/net/DHCP.h>
#include <boot/net/NetDefs.h>
#include <boot/net/RemoteDiskDefs.h>
#include <boot/vfs.h>
#include <OS.h>

#define DHCP_REQESTMSG 1
#define DHCP_REPLYMSG 2

#define DHCP_CLIENT_MSG_TYPE_DHCPDISCOVER 1
#define DHCP_CLIENT_MSG_TYPE_DHCPOFFER 2
#define DHCP_CLIENT_MSG_TYPE_DHCPREQUEST 3
#define DHCP_CLIENT_MSG_TYPE_DHCPDDECLINE 4
#define DHCP_CLIENT_MSG_TYPE_DHCPACK 5

#define DHCP_CLIENT_OPTION_REQUEST_IP 50
#define DHCP_CLIENT_OPTION_DHCP 53
#define DHCP_CLIENT_OPTION_DHCP_SERVER 54
#define DHCP_CLIENT_OPTION_END 255

#define DHCP_CLIENT_SRCPORT 68
#define DHCP_CLIENT_DESTPORT 67

static const bigtime_t kRequestTimeout = 100000LL;

DHCPClient::DHCPClient(EthernetInterface *interface) :
	fInterface(interface),
	fSocket(NULL) 
{
	// allocate a packet buffer. Some architectures have a really small
	// stack size
	fPacketBuffer = (struct dhcp_packet*) malloc(sizeof(struct dhcp_packet));
}

DHCPClient::~DHCPClient()
{
	// incase a buffer has been allocated, free it.
	if (fPacketBuffer != NULL)
		free(fPacketBuffer);
	if (fSocket != NULL)
		free(fSocket);
}

// runs the DHCP sequence. On B_OK, DHCPClient::IPAddress()
status_t DHCPClient::run()
{
	// check if the packet buffer was successfully allocated
	if (fPacketBuffer == NULL)
		return B_NO_MEMORY;

	// create and bind socket
	fSocket = new(nothrow) UDPSocket;
	if (!fSocket)
		return B_NO_MEMORY;
	status_t error = fSocket->Bind(INADDR_ANY, DHCP_CLIENT_SRCPORT);
	if (error != B_OK)
		return error;

	// create a dhcp discover paket
	initializePacketBuffer();

	// set the dhcp message type
	fPacketBuffer->options[fOptionsPos++] = DHCP_CLIENT_OPTION_DHCP;
	fPacketBuffer->options[fOptionsPos++] = 1;
	fPacketBuffer->options[fOptionsPos++] = DHCP_CLIENT_MSG_TYPE_DHCPDISCOVER;

	// end the options field
	fPacketBuffer->options[fOptionsPos++] = DHCP_CLIENT_OPTION_END;
	fPacketBuffer->options[fOptionsPos++] = 0;

	// resend the discover up to 3 times
	ip_addr_t clientAddress = INADDR_NONE;
	ip_addr_t serverAddress = INADDR_NONE;
	for (int i = 0; i < 3; i++) {
		// send off the request
		status_t error = fSocket->Send(INADDR_BROADCAST, DHCP_CLIENT_DESTPORT, fPacketBuffer,
			sizeof(struct dhcp_packet));
		if (error != B_OK)
			return error;

		// receive reply
		bigtime_t timeout = system_time() + kRequestTimeout;
		do {
			UDPPacket* packet;
			error = fSocket->Receive(&packet, timeout - system_time());
			if (error == B_OK) {
				// got something, TODO check it
				if (packet->DataSize() >= sizeof(struct dhcp_packet)) {
					struct dhcp_packet* dhcpPacket = ((dhcp_packet*)packet->Data());
					// don't snatch a packet from a concurrent DHCP request
					if (dhcpPacket->chaddr == fInterface->MACAddress()) {
						// TODO check that this is actually a DHCP offer
						clientAddress = dhcpPacket->yiaddr;
						serverAddress = dhcpPacket->siaddr;
						delete packet;
						goto exit_discoverloop;
					}
				}

				delete packet;

			} else if (error != B_TIMED_OUT && error != B_WOULD_BLOCK) {
				dprintf("DHCPClient::run(): timeout\n");
				return error;
			}
		} while (timeout > system_time());
	}
	return B_TIMED_OUT;

exit_discoverloop:
	
	// create a dhcp request
	initializePacketBuffer();
	fPacketBuffer->siaddr = serverAddress;

	// set the dhcp message type
	fPacketBuffer->options[fOptionsPos++] = DHCP_CLIENT_OPTION_DHCP;
	fPacketBuffer->options[fOptionsPos++] = 1;
	fPacketBuffer->options[fOptionsPos++] = DHCP_CLIENT_MSG_TYPE_DHCPREQUEST;

	// ask for the offered IP address
	fPacketBuffer->options[fOptionsPos++] = DHCP_CLIENT_OPTION_REQUEST_IP;
	fPacketBuffer->options[fOptionsPos++] = 4;
	*((ip_addr_t*)(fPacketBuffer->options + fOptionsPos)) = clientAddress;
	fOptionsPos += 4;

	// set the correct dhcp server
	fPacketBuffer->options[fOptionsPos++] = DHCP_CLIENT_OPTION_DHCP_SERVER;
	fPacketBuffer->options[fOptionsPos++] = 4;
	*((ip_addr_t*)(fPacketBuffer->options + fOptionsPos)) = serverAddress;
	fOptionsPos += 4;

	// end the options field
	fPacketBuffer->options[fOptionsPos++] = DHCP_CLIENT_OPTION_END;
	fPacketBuffer->options[fOptionsPos++] = 0;

	// resend the request up to 3 times
	for (int i = 0; i < 3; i++) {
		// send off the request
		status_t error = fSocket->Send(INADDR_BROADCAST, DHCP_CLIENT_DESTPORT, fPacketBuffer, sizeof(struct dhcp_packet));
		if(error != B_OK)
			return error;

		// receive reply
		bigtime_t timeout = system_time() + kRequestTimeout;
		do {
			UDPPacket* packet;
			error = fSocket->Receive(&packet, timeout - system_time());
			if (error == B_OK) {
				// got something, TODO check it
				if (packet->DataSize() >= sizeof(struct dhcp_packet)) {
					struct dhcp_packet* dhcpPacket = ((dhcp_packet*)packet->Data());

					// don't snatch a packet from a concurrent DHCP request
					if(dhcpPacket->chaddr == fInterface->MACAddress()) {
						// TODO check that this is actually a DHCP ACK
						delete packet;
						goto exit_requestloop;
					}
				}

				// incorrect reply
				delete packet;
			} else if (error != B_TIMED_OUT && error != B_WOULD_BLOCK) {
				dprintf("DHCPClient::run(): timeout\n");
				return error;
			}
		} while (timeout > system_time());
	}
	return B_TIMED_OUT;

exit_requestloop:
	fAddress = clientAddress;
	return B_OK;
}

void
DHCPClient::initializePacketBuffer()
{
	// set common flags
	fPacketBuffer->opcode = DHCP_REQESTMSG;
	fPacketBuffer->htype = htons(ETHERTYPE_IP);
	fPacketBuffer->hlen = DHCP_802ADDR_LEN;
	fPacketBuffer->hops = 0;
	fPacketBuffer->xid = xid_;
	fPacketBuffer->flags = htons(0x8000);
	fPacketBuffer->yiaddr = INADDR_ANY;
	fPacketBuffer->ciaddr = INADDR_ANY;
	fPacketBuffer->siaddr = INADDR_ANY;
	fPacketBuffer->giaddr = INADDR_ANY;
	memset(fPacketBuffer->chaddr_bytes, '\0', DHCP_CHADDR_LEN);
	fPacketBuffer->chaddr = fInterface->MACAddress();
	memset(fPacketBuffer->sname, '\0', DHCP_CLIENT_SNAME_LEN);
	memset(fPacketBuffer->file, '\0', DHCP_CLIENT_FILE_LEN);
	memset(fPacketBuffer->options, '\0', DHCP_CLIENT_OPTIONS_LEN);

	// set the secs field
	// TODO use the system time here
	fPacketBuffer->secs = 0;

	// write the magic cookie
	fOptionsPos = 0;
	fPacketBuffer->options[fOptionsPos++] = 99;
	fPacketBuffer->options[fOptionsPos++] = 130;
	fPacketBuffer->options[fOptionsPos++] = 83;
	fPacketBuffer->options[fOptionsPos++] = 99;
}

ip_addr_t
DHCPClient::IPAddress() const
{
	return fAddress;
}
