/*
 * Copyright 2010-2020 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _B_URL_SESSION_H_
#define _B_URL_SESSION_H_


#include <Certificate.h>
#include <HttpAuthentication.h>
#include <NetworkCookieJar.h>


class BUrlRequest;
class BUrlProtocolListener;


class BUrlProtocolSession {
public:
	virtual	~BUrlProtocolSession() = 0;
};


class BUrlSession {
public:
							BUrlSession();
							~BUrlSession();

	status_t				InitCheck() const;

	// Session modifiers
	void					SetCookieJar(
								const BNetworkCookieJar& cookieJar);
	status_t				AddAuthentication(const BUrl& url,
								const BHttpAuthentication& auth);
	void					SetProxy(BString host, uint16 port);
	status_t				AddCertificateException(
								const BCertificate& certificate);

	// Session accessors
	BNetworkCookieJar&		GetCookieJar();
	BHttpAuthentication&	GetAuthentication(const BUrl& url);
	bool					UseProxy();
	BString					GetProxyHost();
	uint16					GetProxyPort();
	bool					HasCertificateException(
								const BCertificate& certificate);
	BUrlProtocolSession*	GetProtocolSession(
								const char* protocol);

	BUrlRequest*			MakeRequest(const BUrl& url,
								BDataIO* output,
								BUrlProtocolListener* listener = NULL);

private:
							BUrlSession(const BUrlSession&);

private:
	class					BHttpAuthenticationMap;
	class					BProtocolSessionMap;
	typedef BObjectList<const BCertificate> BCertificateSet;
	friend class BUrlRequest;

private:
	BNetworkCookieJar		fCookieJar;
	BHttpAuthenticationMap*	fAuthenticationMap;
	BCertificateSet			fCertificates;
	BProtocolSessionMap*	fSessionMap;

	status_t				fInitStatus;

	BString					fProxyHost;
	uint16					fProxyPort;
};


#endif
