/*
 * Copyright 2010-2014 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_REQUEST_H_
#define _B_URL_REQUEST_H_


#include <Url.h>
#include <UrlProtocolListener.h>
#include <UrlResult.h>
#include <UrlSession.h>
#include <OS.h>
#include <Referenceable.h>


class BUrlRequest {
public:
									BUrlRequest(BUrlSession& session,
										const BUrl& url,
										BDataIO* output,
										BUrlProtocolListener* listener,
										const char* threadName,
										const char* protocolName);
	virtual							~BUrlRequest();

	// URL protocol thread management
	virtual	thread_id				Run();
	virtual	status_t				Pause();
	virtual	status_t				Resume();
	virtual	status_t				Stop();
	virtual	void					SetTimeout(bigtime_t timeout) {}
	virtual	status_t				WaitForCompletion(uint32 flags = 0,
										bigtime_t timeout = B_INFINITE_TIMEOUT);

	// URL protocol parameters modification
			status_t				SetUrl(const BUrl& url);
			status_t				SetListener(BUrlProtocolListener* listener);
			status_t				SetOutput(BDataIO* output);

	// URL protocol parameters access
			const BUrl&				Url() const;
			BUrlSession&			Session() const;
			BUrlProtocolListener*	Listener() const;
			const BString&			Protocol() const;
			BDataIO*				Output() const;

	// URL protocol informations
			bool					IsRunning() const;
			status_t				Status() const;
	virtual	const BUrlResult&		Result() const = 0;


protected:
	static	int32					_ThreadEntry(void* arg);
	virtual	void					_ProtocolSetup() {};
	virtual	status_t				_ProtocolLoop() = 0;
	virtual	void					_EmitDebug(BUrlProtocolDebugMessage type,
										const char* format, ...);
protected:
			BUrl					fUrl;
			BUrlSession*			fSession;
			BUrlProtocolListener*	fListener;
			BDataIO*				fOutput;

			bool					fQuit;
			bool					fRunning;
			status_t				fThreadStatus;
			thread_id				fThreadId;
			BString					fThreadName;
			BString					fProtocol;
};


#endif // _B_URL_REQUEST_H_
