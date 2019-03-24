/*
 * Copyright 2010-2017 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */

#include <iostream>
#include <cstdio>

#include <UrlRequest.h>
#include <UrlProtocolListener.h>

using namespace std;


void
BURLProtocolListener::ConnectionOpened(BURLRequest*)
{
}


void
BURLProtocolListener::HostnameResolved(BURLRequest*, const char*)
{
}


bool
BURLProtocolListener::CertificateVerificationFailed(BURLRequest* caller,
	BCertificate& certificate, const char* message)
{
	return false;
}


void
BURLProtocolListener::ResponseStarted(BURLRequest*)
{
}


void
BURLProtocolListener::HeadersReceived(BURLRequest*, const BURLResult& result)
{
}


void
BURLProtocolListener::DataReceived(BURLRequest*, const char*, off_t, ssize_t)
{
}


void
BURLProtocolListener::DownloadProgress(BURLRequest*, ssize_t, ssize_t)
{
}


void
BURLProtocolListener::UploadProgress(BURLRequest*, ssize_t, ssize_t)
{
}


void
BURLProtocolListener::RequestCompleted(BURLRequest*, bool)
{
}


void
BURLProtocolListener::DebugMessage(BURLRequest* caller,
	BURLProtocolDebugMessage type, const char* text)
{
#ifdef DEBUG
	switch (type) {
		case B_URL_PROTOCOL_DEBUG_TEXT:
			cout << "   ";
			break;
			
		case B_URL_PROTOCOL_DEBUG_ERROR:
			cout << "!!!";
			break;
			
		case B_URL_PROTOCOL_DEBUG_TRANSFER_IN:
		case B_URL_PROTOCOL_DEBUG_HEADER_IN:
			cout << "<--";
			break;
			
		case B_URL_PROTOCOL_DEBUG_TRANSFER_OUT:
		case B_URL_PROTOCOL_DEBUG_HEADER_OUT:
			cout << "-->";
			break;
	}
	
	cout << " " << caller->Protocol() << ": " << text << endl;
#endif
}
