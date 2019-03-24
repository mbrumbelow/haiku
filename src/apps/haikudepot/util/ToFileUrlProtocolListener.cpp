/*
 * Copyright 2017-2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ToFileUrlProtocolListener.h"

#include <File.h>
#include <HttpRequest.h>

#include <stdio.h>


ToFileUrlProtocolListener::ToFileUrlProtocolListener(BPath path,
	BString traceLoggingIdentifier, bool traceLogging)
{
	fDownloadIO = new BFile(path.Path(), O_WRONLY | O_CREAT);
	fTraceLoggingIdentifier = traceLoggingIdentifier;
	fTraceLogging = traceLogging;
	fShouldDownload = true;
	fContentLength = 0;
}


ToFileUrlProtocolListener::~ToFileUrlProtocolListener()
{
	delete fDownloadIO;
}


void
ToFileUrlProtocolListener::ConnectionOpened(BURLRequest* caller)
{
}


void
ToFileUrlProtocolListener::HostnameResolved(BURLRequest* caller,
	const char* ip)
{
}


void
ToFileUrlProtocolListener::ResponseStarted(BURLRequest* caller)
{
}


void
ToFileUrlProtocolListener::HeadersReceived(BURLRequest* caller,
	const BURLResult& result)
{

	// check that the status code is success.  Only if it is successful
	// should the payload be streamed to the file.

	const BHttpResult& httpResult = dynamic_cast<const BHttpResult&>(result);
	int32 statusCode = httpResult.StatusCode();

	if (!BHttpRequest::IsSuccessStatusCode(statusCode)) {
		fprintf(stdout, "received http status %" B_PRId32
			" --> will not store download to file\n", statusCode);
		fShouldDownload = false;
	}

}


void
ToFileUrlProtocolListener::DataReceived(BURLRequest* caller, const char* data,
	off_t position, ssize_t size)
{
	fContentLength += size;

	if (fShouldDownload && fDownloadIO != NULL && size > 0) {
		size_t remaining = size;
		size_t written = 0;

		do {
			written = fDownloadIO->WriteAt(position, &data[size - remaining],
				remaining);
			remaining -= written;
		} while (remaining > 0 && written > 0);

		if (remaining > 0)
			fprintf(stdout, "unable to write all of the data to the file\n");
	}
}


void
ToFileUrlProtocolListener::DownloadProgress(BURLRequest* caller,
	ssize_t bytesReceived, ssize_t bytesTotal)
{
}


void
ToFileUrlProtocolListener::UploadProgress(BURLRequest* caller,
	ssize_t bytesSent, ssize_t bytesTotal)
{
}


void
ToFileUrlProtocolListener::RequestCompleted(BURLRequest* caller, bool success)
{
}


void
ToFileUrlProtocolListener::DebugMessage(BURLRequest* caller,
	BURLProtocolDebugMessage type, const char* text)
{
	if (fTraceLogging) {
		fprintf(stdout, "url->file <%s>; %s\n",
			fTraceLoggingIdentifier.String(), text);
	}
}


ssize_t
ToFileUrlProtocolListener::ContentLength()
{
	return fContentLength;
}
