/*
 * Copyright 2010-2020 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 *		Leorize, leorize+oss@disroot.org
 */


#include <UrlSession.h>

#include <stdio.h>

#include <AutoLocker.h>
#include <DataRequest.h>
#include <Debug.h>
#include <FileRequest.h>
#include <GopherRequest.h>
#include <HashMap.h>
#include <HashString.h>
#include <HttpRequest.h>
#include <UrlRequest.h>


class BUrlSession::BHttpAuthenticationMap : public
	SynchronizedHashMap<BPrivate::HashString, BHttpAuthentication*> {};


class BUrlSession::BProtocolSessionMap : public
	SynchronizedHashMap<BPrivate::HashString, BUrlProtocolSession*> {};


BUrlSession::BUrlSession()
	:
	fCookieJar(),
	fAuthenticationMap(NULL),
	fCertificates(20, true),
	fSessionMap(NULL),
	fInitStatus(B_NO_INIT),
	fProxyHost(),
	fProxyPort(0)
{
	fAuthenticationMap = new(std::nothrow) BHttpAuthenticationMap();

	if (fAuthenticationMap == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}

	fInitStatus = fAuthenticationMap->InitCheck();
	if (fInitStatus != B_OK)
		return;

	// This is the default authentication, used when nothing else is found.
	// The empty string used as a key will match all the domain strings,
	// once we have removed all components.
	BHttpAuthentication* defaultAuth = new(std::nothrow) BHttpAuthentication();
	if (defaultAuth == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}

	fInitStatus = fAuthenticationMap->Put(HashString("", 0), defaultAuth);
	if (fInitStatus != B_OK)
		return;

	fSessionMap = new(std::nothrow) BProtocolSessionMap();
	if (fSessionMap == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}

	fInitStatus = fSessionMap->InitCheck();
}


BUrlSession::~BUrlSession()
{
	BHttpAuthenticationMap::Iterator iterator
		= fAuthenticationMap->GetIterator();
	while (iterator.HasNext())
		delete iterator.Next().value;

	delete fAuthenticationMap;
	delete fSessionMap;
}


status_t
BUrlSession::InitCheck() const
{
	return fInitStatus;
}


// #pragma mark Context modifiers


void
BUrlSession::SetCookieJar(const BNetworkCookieJar& cookieJar)
{
	fCookieJar = cookieJar;
}


status_t
BUrlSession::AddAuthentication(const BUrl& url,
	const BHttpAuthentication& authentication)
{
	BString domain = url.Host();
	domain += url.Path();
	HashString hostHash(domain.String(), domain.Length());

	AutoLocker<BHttpAuthenticationMap> _(fAuthenticationMap);

	BHttpAuthentication* previous = fAuthenticationMap->Get(hostHash);

	if (previous != NULL)
		*previous = authentication;
	else {
		BHttpAuthentication* copy
			= new(std::nothrow) BHttpAuthentication(authentication);
		if (copy == NULL)
			return B_NO_MEMORY;
		status_t err = fAuthenticationMap->Put(hostHash, copy);
		if (err != B_OK) {
			delete copy;
			return err;
		}
	}

	return B_OK;
}


void
BUrlSession::SetProxy(BString host, uint16 port)
{
	fProxyHost = host;
	fProxyPort = port;
}


status_t
BUrlSession::AddCertificateException(const BCertificate& certificate)
{
	BCertificate* copy = new(std::nothrow) BCertificate(certificate);
	if (copy == NULL)
		return B_NO_MEMORY;

	if (!fCertificates.AddItem(copy)) {
		delete copy;
		return B_NO_MEMORY;
	}

	return B_OK;
}


// #pragma mark Context accessors


BNetworkCookieJar&
BUrlSession::GetCookieJar()
{
	return fCookieJar;
}


BHttpAuthentication&
BUrlSession::GetAuthentication(const BUrl& url)
{
	BString domain = url.Host();
	domain += url.Path();

	BHttpAuthentication* authentication = NULL;

	do {
		authentication = fAuthenticationMap->Get(HashString(domain.String(),
			domain.Length()));

		domain.Truncate(domain.FindLast('/'));

	} while (authentication == NULL);

	return *authentication;
}


bool
BUrlSession::UseProxy()
{
	return !fProxyHost.IsEmpty();
}


BString
BUrlSession::GetProxyHost()
{
	return fProxyHost;
}


uint16
BUrlSession::GetProxyPort()
{
	return fProxyPort;
}


bool
BUrlSession::HasCertificateException(const BCertificate& certificate)
{
	struct Equals: public UnaryPredicate<const BCertificate> {
		Equals(const BCertificate& itemToMatch)
			:
			fItemToMatch(itemToMatch)
		{
		}

		int operator()(const BCertificate* item) const
		{
			/* Must return 0 if there is a match! */
			return !(*item == fItemToMatch);
		}

		const BCertificate& fItemToMatch;
	} comparator(certificate);

	return fCertificates.FindIf(comparator) != NULL;
}


BUrlProtocolSession*
BUrlSession::GetProtocolSession(const char* protocol)
{
	AutoLocker<BProtocolSessionMap> _(fSessionMap);

	BUrlProtocolSession* session = fSessionMap->Get(HashString(protocol));
	if (session != NULL)
		return session;

	// WIP create the session for the given protocol if not exist yet.
	// This part should also be done via the add-on interface.

	return NULL;
}


BUrlRequest*
BUrlSession::MakeRequest(const BUrl& url, BDataIO* output,
	BUrlProtocolListener* listener)
{
	// TODO: instantiate the correct BUrlProtocol using add-on interface
	if (url.Protocol() == "http") {
		return new(std::nothrow) BHttpRequest(*this, url, output, false,
			"HTTP", listener);
	} else if (url.Protocol() == "https") {
		return new(std::nothrow) BHttpRequest(*this, url, output, true,
			"HTTPS", listener);
	} else if (url.Protocol() == "file") {
		return new(std::nothrow) BFileRequest(*this, url, output, listener);
	} else if (url.Protocol() == "data") {
		return new(std::nothrow) BDataRequest(*this, url, output, listener);
	} else if (url.Protocol() == "gopher") {
		return new(std::nothrow) BGopherRequest(*this, url, output, listener);
	}

	return NULL;
}
