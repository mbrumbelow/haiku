/*
 * Copyright 2013 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_FILE_REQUEST_H_
#define _B_FILE_REQUEST_H_


#include <deque>


#include <UrlRequest.h>
#include <UrlSession.h>


class BFileRequest : public BUrlRequest {
public:
	virtual						~BFileRequest();

	const 	BUrlResult&			Result() const;
			void				SetDisableListener(bool disable);

private:
			friend class BUrlSession;

								BFileRequest(BUrlSession& session,
									const BUrl& url,
									BDataIO* output,
									BUrlProtocolListener* listener = NULL);

			status_t			_ProtocolLoop();
private:
			BUrlResult			fResult;
};


#endif // _B_FILE_REQUEST_H_
