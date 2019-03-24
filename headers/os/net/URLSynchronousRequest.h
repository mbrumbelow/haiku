/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_SYNCHRONOUS_REQUEST_H_
#define _B_URL_SYNCHRONOUS_REQUEST_H_


#include <UrlRequest.h>
#include <UrlProtocolListener.h>


class BURLSynchronousRequest : public BURLRequest, public BURLProtocolListener {
public:
								BURLSynchronousRequest(BURLRequest& asynchronousRequest);
	virtual						~BURLSynchronousRequest() { };
								
	// Synchronous wait
	virtual	status_t			Perform();
	virtual	status_t			WaitUntilCompletion();

	// Protocol hooks
	virtual	void				ConnectionOpened(BURLRequest* caller);
	virtual void				HostnameResolved(BURLRequest* caller,
									const char* ip);
	virtual void				ResponseStarted(BURLRequest* caller);
	virtual void				HeadersReceived(BURLRequest* caller,
									const BURLResult& result);
	virtual void				DataReceived(BURLRequest* caller,
									const char* data, off_t position,
									ssize_t size);
	virtual	void				DownloadProgress(BURLRequest* caller,
									ssize_t bytesReceived, ssize_t bytesTotal);
	virtual void				UploadProgress(BURLRequest* caller,
									ssize_t bytesSent, ssize_t bytesTotal);
	virtual void				RequestCompleted(BURLRequest* caller,
									bool success);
									
									
protected:
			bool				fRequestComplete;
			BURLRequest&		fWrappedRequest;
};


#endif // _B_URL_SYNCHRONOUS_REQUEST_H_
