/*
 * Copyright 2006-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYS_UN_H
#define _SYS_UN_H


#include <sys/socket.h>


struct sockaddr_un {
	uint8_t		sun_len;
	uint8_t		sun_family;
	char		sun_path[126];
};

/* actual length of an initialized sockaddr_un */
#define SUN_LEN(su) \
    (sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path))

#endif	/* _SYS_UN_H */
