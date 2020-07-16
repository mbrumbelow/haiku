/*
 * Copyright 2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#ifndef _B_DATA_REQUEST_H_
#define _B_DATA_REQUEST_H_


#include <UrlProtocolRoster.h>
#include <UrlRequest.h>


class BDataRequest: public BUrlRequest {
	friend class BUrlProtocolRoster;
public:
		const BUrlResult&	Result() const;
private:
							BDataRequest(const BUrl& url,
								BUrlProtocolListener* listener = NULL,
								BUrlContext* context = NULL);

		status_t			_ProtocolLoop();
private:
		BUrlResult			fResult;
};

#endif
