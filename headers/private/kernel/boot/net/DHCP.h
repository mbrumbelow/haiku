/*
 * Copyright 2025, Maite Gamper <mail@zeldakatze.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
 
#ifndef _BOOT_DHCP_H
#define _BOOT_DHCP_H

#include <boot/net/Ethernet.h>
#include <boot/net/UDP.h>

#define DHCP_802ADDR_LEN 6
#define DHCP_CHADDR_LEN 16
#define DHCP_CLIENT_SNAME_LEN    64
#define DHCP_CLIENT_FILE_LEN    128
#define DHCP_CLIENT_OPTIONS_LEN 312

// the structure of a DHCP packet, like it's sent over ethernet
struct dhcp_packet {
	uint8 opcode, htype, hlen, hops;
	uint32 xid;
	uint16 secs, flags;
	ip_addr_t ciaddr, yiaddr, siaddr, giaddr;
	union {
			char chaddr_bytes[DHCP_CHADDR_LEN];
			mac_addr_t chaddr;
	} __attribute__ ((__packed__));
	char sname[DHCP_CLIENT_SNAME_LEN];
	char file[DHCP_CLIENT_FILE_LEN];
	char options[DHCP_CLIENT_OPTIONS_LEN];
} __attribute__ ((__packed__));

class DHCPClient {
	public:
		DHCPClient(EthernetInterface *interface);
		~DHCPClient();
		status_t run();
		ip_addr_t IPAddress() const;
	
	private:
		EthernetInterface *fInterface;
		ip_addr_t fAddress;
		struct dhcp_packet *fPacketBuffer;
		int fStartTime;
		uint32 xid_;
		
		int fOptionsPos;
		UDPSocket	*fSocket;
		
		void initializePacketBuffer();
		
};

#endif // _BOOT_DHCP_H
