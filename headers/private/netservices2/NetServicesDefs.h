/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _NETSERVICES_DEFS_H_
#define _NETSERVICES_DEFS_H_


#include <StringList.h>
#include <Url.h>


namespace BPrivate {

namespace Network {


// Standard exceptions
struct unsupported_protocol_exception {
	BUrl		url;
	BStringList	supportedProtocols;
};


struct invalid_url_exception {
	BUrl	url;
};


}

}

#endif
