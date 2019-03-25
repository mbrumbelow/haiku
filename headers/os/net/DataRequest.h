/*
 * Copyright 2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#ifndef _B_DATA_REQUEST_H_
#define _B_DATA_REQUEST_H_


#include <UrlRequest.h>


class BDataRequest: public BURLRequest {
public:
							BDataRequest(const BURL& url,
								BURLProtocolListener* listener = NULL,
								BURLContext* context = NULL);
		const BURLResult&	Result() const;
private:
		status_t			_ProtocolLoop();	
private:
		BURLResult			fResult;
};

#endif
