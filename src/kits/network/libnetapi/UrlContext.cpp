/*
 * Copyright 2010-2015 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include <UrlContext.h>

#include <stdio.h>

#include <HashMap.h>
#include <HashString.h>


class BURLContext::BHttpAuthenticationMap : public
	SynchronizedHashMap<BPrivate::HashString, BHttpAuthentication*> {};


BURLContext::BURLContext()
	:
	fCookieJar(),
	fAuthenticationMap(NULL),
	fCertificates(20, true),
	fProxyHost(),
	fProxyPort(0)
{
	fAuthenticationMap = new(std::nothrow) BHttpAuthenticationMap();

	// This is the default authentication, used when nothing else is found.
	// The empty string used as a key will match all the domain strings, once
	// we have removed all components.
	fAuthenticationMap->Put(HashString("", 0), new BHttpAuthentication());
}


BURLContext::~BURLContext()
{
	BHttpAuthenticationMap::Iterator iterator
		= fAuthenticationMap->GetIterator();
	while (iterator.HasNext())
		delete iterator.Next().value;

	delete fAuthenticationMap;
}


// #pragma mark Context modifiers


void
BURLContext::SetCookieJar(const BNetworkCookieJar& cookieJar)
{
	fCookieJar = cookieJar;
}


void
BURLContext::AddAuthentication(const BURL& url,
	const BHttpAuthentication& authentication)
{
	BString domain = url.Host();
	domain += url.Path();
	BPrivate::HashString hostHash(domain.String(), domain.Length());

	fAuthenticationMap->Lock();

	BHttpAuthentication* previous = fAuthenticationMap->Get(hostHash);

	if (previous)
		*previous = authentication;
	else {
		BHttpAuthentication* copy
			= new(std::nothrow) BHttpAuthentication(authentication);
		fAuthenticationMap->Put(hostHash, copy);
	}

	fAuthenticationMap->Unlock();
}


void
BURLContext::SetProxy(BString host, uint16 port)
{
	fProxyHost = host;
	fProxyPort = port;
}


void
BURLContext::AddCertificateException(const BCertificate& certificate)
{
	BCertificate* copy = new(std::nothrow) BCertificate(certificate);
	if (copy != NULL) {
		fCertificates.AddItem(copy);
	}
}


// #pragma mark Context accessors


BNetworkCookieJar&
BURLContext::GetCookieJar()
{
	return fCookieJar;
}


BHttpAuthentication&
BURLContext::GetAuthentication(const BURL& url)
{
	BString domain = url.Host();
	domain += url.Path();

	BHttpAuthentication* authentication = NULL;

	do {
		authentication = fAuthenticationMap->Get( HashString(domain.String(),
			domain.Length()));

		domain.Truncate(domain.FindLast('/'));

	} while (authentication == NULL);

	return *authentication;
}


bool
BURLContext::UseProxy()
{
	return !fProxyHost.IsEmpty();
}


BString
BURLContext::GetProxyHost()
{
	return fProxyHost;
}


uint16
BURLContext::GetProxyPort()
{
	return fProxyPort;
}


bool
BURLContext::HasCertificateException(const BCertificate& certificate)
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
