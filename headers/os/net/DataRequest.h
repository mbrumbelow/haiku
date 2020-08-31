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
#include <UrlSession.h>


class BDataRequest: public BUrlRequest {
public:
		const BUrlResult&	Result() const;
private:
		friend class BUrlSession;

							BDataRequest(BUrlSession& session,
								const BUrl& url,
								BDataIO* output,
								BUrlProtocolListener* listener = NULL);

		status_t			_ProtocolLoop();
private:
		BUrlResult			fResult;
};

#endif
