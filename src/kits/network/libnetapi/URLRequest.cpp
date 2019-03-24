/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 */


#include <UrlRequest.h>
#include <Debug.h>
#include <stdio.h>


static BReference<BURLContext> gDefaultContext = new(std::nothrow) BURLContext();


BURLRequest::BURLRequest(const BURL& url, BURLProtocolListener* listener,
	BURLContext* context, const char* threadName, const char* protocolName)
	:
	fUrl(url),
	fContext(context),
	fListener(listener),
	fQuit(false),
	fRunning(false),
	fThreadStatus(B_NO_INIT),
	fThreadId(0),
	fThreadName(threadName),
	fProtocol(protocolName)
{
	if (fContext == NULL)
		fContext = gDefaultContext;
}


BURLRequest::~BURLRequest()
{
	Stop();
}


// #pragma mark URL protocol thread management


thread_id
BURLRequest::Run()
{
	// Thread already running
	if (fRunning) {
		PRINT(("BURLRequest::Run() : Oops, already running ! "
			"[urlProtocol=%p]!\n", this));
		return fThreadId;
	}

	fThreadId = spawn_thread(BURLRequest::_ThreadEntry, fThreadName,
		B_NORMAL_PRIORITY, this);

	if (fThreadId < B_OK)
		return fThreadId;

	fRunning = true;

	status_t launchErr = resume_thread(fThreadId);
	if (launchErr < B_OK) {
		PRINT(("BURLRequest::Run() : Failed to resume thread %" B_PRId32 "\n",
			fThreadId));
		return launchErr;
	}

	return fThreadId;
}


status_t
BURLRequest::Pause()
{
	// TODO
	return B_ERROR;
}


status_t
BURLRequest::Resume()
{
	// TODO
	return B_ERROR;
}


status_t
BURLRequest::Stop()
{
	if (!fRunning)
		return B_ERROR;

	fQuit = true;
	return B_OK;
}


// #pragma mark URL protocol parameters modification


status_t
BURLRequest::SetUrl(const BURL& url)
{
	// We should avoid to change URL while the thread is running ...
	if (IsRunning())
		return B_ERROR;

	fUrl = url;
	return B_OK;
}


status_t
BURLRequest::SetContext(BURLContext* context)
{
	if (IsRunning())
		return B_ERROR;

	fContext = context;
	return B_OK;
}


status_t
BURLRequest::SetListener(BURLProtocolListener* listener)
{
	if (IsRunning())
		return B_ERROR;

	fListener = listener;
	return B_OK;
}


// #pragma mark URL protocol parameters access


const BURL&
BURLRequest::Url() const
{
	return fUrl;
}


BURLContext*
BURLRequest::Context() const
{
	return fContext;
}


BURLProtocolListener*
BURLRequest::Listener() const
{
	return fListener;
}


const BString&
BURLRequest::Protocol() const
{
	return fProtocol;
}


// #pragma mark URL protocol informations


bool
BURLRequest::IsRunning() const
{
	return fRunning;
}


status_t
BURLRequest::Status() const
{
	return fThreadStatus;
}


// #pragma mark Thread management


/*static*/ int32
BURLRequest::_ThreadEntry(void* arg)
{
	BURLRequest* request = reinterpret_cast<BURLRequest*>(arg);
	request->fThreadStatus = B_BUSY;
	request->_ProtocolSetup();

	status_t protocolLoopExitStatus = request->_ProtocolLoop();

	request->fRunning = false;
	request->fThreadStatus = protocolLoopExitStatus;

	if (request->fListener != NULL) {
		request->fListener->RequestCompleted(request,
			protocolLoopExitStatus == B_OK);
	}

	return B_OK;
}


void
BURLRequest::_EmitDebug(BURLProtocolDebugMessage type,
	const char* format, ...)
{
	if (fListener == NULL)
		return;

	va_list arguments;
	va_start(arguments, format);

	char debugMsg[1024];
	vsnprintf(debugMsg, sizeof(debugMsg), format, arguments);
	fListener->DebugMessage(this, type, debugMsg);
	va_end(arguments);
}
