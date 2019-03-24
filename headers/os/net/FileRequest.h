/*
 * Copyright 2013 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_FILE_REQUEST_H_
#define _B_FILE_REQUEST_H_


#include <deque>


#include <UrlRequest.h>


class BFileRequest : public BURLRequest {
public:
								BFileRequest(const BURL& url,
									BURLProtocolListener* listener = NULL,
									BURLContext* context = NULL);
	virtual						~BFileRequest();

	const 	BURLResult&			Result() const;
            void                SetDisableListener(bool disable);

private:
			status_t			_ProtocolLoop();
private:
			BURLResult			fResult;
};


#endif // _B_FILE_REQUEST_H_
