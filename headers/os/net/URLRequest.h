/*
 * Copyright 2010-2014 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_REQUEST_H_
#define _B_URL_REQUEST_H_


#include <Url.h>
#include <UrlContext.h>
#include <UrlProtocolListener.h>
#include <UrlResult.h>
#include <OS.h>
#include <Referenceable.h>


class BURLRequest {
public:
									BURLRequest(const BURL& url,
										BURLProtocolListener* listener,
										BURLContext* context,
										const char* threadName,
										const char* protocolName);
	virtual							~BURLRequest();

	// URL protocol thread management
	virtual	thread_id				Run();
	virtual status_t				Pause();
	virtual status_t				Resume();
	virtual	status_t				Stop();
	virtual void					SetTimeout(bigtime_t timeout) {}

	// URL protocol parameters modification
			status_t				SetUrl(const BURL& url);
			status_t				SetContext(BURLContext* context);
			status_t				SetListener(BURLProtocolListener* listener);

	// URL protocol parameters access
			const BURL&				Url() const;
			BURLContext*			Context() const;
			BURLProtocolListener*	Listener() const;
			const BString&			Protocol() const;

	// URL protocol informations
			bool					IsRunning() const;
			status_t				Status() const;
	virtual const BURLResult&		Result() const = 0;


protected:
	static	int32					_ThreadEntry(void* arg);
	virtual	void					_ProtocolSetup() {};
	virtual	status_t				_ProtocolLoop() = 0;
	virtual void					_EmitDebug(BURLProtocolDebugMessage type,
										const char* format, ...);
protected:
			BURL					fUrl;
			BReference<BURLContext>	fContext;
			BURLProtocolListener*	fListener;

			bool					fQuit;
			bool					fRunning;
			status_t				fThreadStatus;
			thread_id				fThreadId;
			BString					fThreadName;
			BString					fProtocol;
};


#endif // _B_URL_REQUEST_H_
