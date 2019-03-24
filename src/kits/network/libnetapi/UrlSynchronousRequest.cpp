/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 */

#include <cstdio>

#include <UrlSynchronousRequest.h>

#define PRINT(x) printf x;


BURLSynchronousRequest::BURLSynchronousRequest(BURLRequest& request)
	:
	BURLRequest(request.Url(), NULL, request.Context(),
		"BURLSynchronousRequest", request.Protocol()),
	fRequestComplete(false),
	fWrappedRequest(request)
{
}


status_t
BURLSynchronousRequest::Perform()
{
	fWrappedRequest.SetListener(this);
	fRequestComplete = false;

	thread_id worker = fWrappedRequest.Run();
		// TODO something to do with the thread_id maybe ?

	if (worker < B_OK)
		return worker;
	else
		return B_OK;
}


status_t
BURLSynchronousRequest::WaitUntilCompletion()
{
	while (!fRequestComplete)
		snooze(10000);

	return B_OK;
}


void
BURLSynchronousRequest::ConnectionOpened(BURLRequest*)
{
	PRINT(("SynchronousRequest::ConnectionOpened()\n"));
}


void
BURLSynchronousRequest::HostnameResolved(BURLRequest*, const char* ip)
{
	PRINT(("SynchronousRequest::HostnameResolved(%s)\n", ip));
}


void
BURLSynchronousRequest::ResponseStarted(BURLRequest*)
{
	PRINT(("SynchronousRequest::ResponseStarted()\n"));
}


void
BURLSynchronousRequest::HeadersReceived(BURLRequest*, const BURLResult& result)
{
	PRINT(("SynchronousRequest::HeadersReceived()\n"));
}


void
BURLSynchronousRequest::DataReceived(BURLRequest*, const char*,
	off_t, ssize_t size)
{
	PRINT(("SynchronousRequest::DataReceived(%zd)\n", size));
}


void
BURLSynchronousRequest::DownloadProgress(BURLRequest*,
	ssize_t bytesReceived, ssize_t bytesTotal)
{
	PRINT(("SynchronousRequest::DownloadProgress(%zd, %zd)\n", bytesReceived,
		bytesTotal));
}


void
BURLSynchronousRequest::UploadProgress(BURLRequest*, ssize_t bytesSent,
	ssize_t bytesTotal)
{
	PRINT(("SynchronousRequest::UploadProgress(%zd, %zd)\n", bytesSent,
		bytesTotal));
}


void
BURLSynchronousRequest::RequestCompleted(BURLRequest* caller, bool success)
{
	PRINT(("SynchronousRequest::RequestCompleted(%s) : %s\n", (success?"true":"false"),
		strerror(caller->Status())));
	fRequestComplete = true;
}
