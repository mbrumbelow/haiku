/*
 * Copyright 2013 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _B_URL_ROSTER_H_
#define _B_URL_ROSTER_H_


#include <stdlib.h>
#include <Path.h>
#include <SupportDefs.h>

class BUrl;
class BUrlContext;
class BUrlProtocolListener;
class BUrlRequest;

typedef BUrlRequest* create_uri_addon(const BUrl& url,
        BUrlProtocolListener* listener, BUrlContext* context);

class BUrlProtocolRoster {
public:
	static BUrlRequest*		MakeRequest(const BUrl& url,
								BUrlProtocolListener* listener = NULL,
								BUrlContext* context = NULL);
private:
	static create_uri_addon* _LoadAddOnProtocol(BPath path);

	static BUrlRequest*		_ScanAddOnProtocols(const BUrl& url,
								BUrlProtocolListener* listener, BUrlContext* context);
};

#endif
