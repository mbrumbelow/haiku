/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <UrlProtocolListener.h>
#include <UrlRequest.h>

class ToFileUrlProtocolListener : public BURLProtocolListener {
public:
								ToFileUrlProtocolListener(BPath path,
									BString traceLoggingIdentifier,
									bool traceLogging);
			virtual				~ToFileUrlProtocolListener();

			ssize_t				ContentLength();

			void				ConnectionOpened(BURLRequest* caller);
			void				HostnameResolved(BURLRequest* caller,
									const char* ip);
			void				ResponseStarted(BURLRequest* caller);
			void				HeadersReceived(BURLRequest* caller,
									const BURLResult& result);
			void				DataReceived(BURLRequest* caller,
									const char* data, off_t position,
									ssize_t size);
			void				DownloadProgress(BURLRequest* caller,
									ssize_t bytesReceived, ssize_t bytesTotal);
			void				UploadProgress(BURLRequest* caller,
									ssize_t bytesSent, ssize_t bytesTotal);
			void				RequestCompleted(BURLRequest* caller,
									bool success);
			void				DebugMessage(BURLRequest* caller,
									BURLProtocolDebugMessage type,
									const char* text);

private:
			bool				fShouldDownload;
			bool				fTraceLogging;
			BString				fTraceLoggingIdentifier;
			BPositionIO*		fDownloadIO;
			ssize_t				fContentLength;


};
