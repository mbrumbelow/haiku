/*
 * Copyright 2013 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _B_URL_ROSTER_H_
#define _B_URL_ROSTER_H_


#include <stdlib.h>


class BURL;
class BURLContext;
class BURLProtocolListener;
class BURLRequest;

class BURLProtocolRoster {
public:
    static  BURLRequest*    MakeRequest(const BURL& url,
		                        BURLProtocolListener* listener = NULL,
                                BURLContext* context = NULL);
};

#endif
