/*
 * Copyright 2010-2015 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#ifndef _B_URL_CONTEXT_H_
#define _B_URL_CONTEXT_H_


#include <Certificate.h>
#include <HttpAuthentication.h>
#include <NetworkCookieJar.h>
#include <Referenceable.h>


class BURLContext: public BReferenceable {
public:
								BURLContext();
								~BURLContext();

	// Context modifiers
			void				SetCookieJar(
									const BNetworkCookieJar& cookieJar);
			void				AddAuthentication(const BURL& url,
									const BHttpAuthentication& authentication);
			void				SetProxy(BString host, uint16 port);
			void				AddCertificateException(const BCertificate& certificate);

	// Context accessors
			BNetworkCookieJar&	GetCookieJar();
			BHttpAuthentication& GetAuthentication(const BURL& url);
			bool				UseProxy();
			BString				GetProxyHost();
			uint16				GetProxyPort();
			bool				HasCertificateException(const BCertificate& certificate);

private:
			class 				BHttpAuthenticationMap;

private:
			BNetworkCookieJar	fCookieJar;
			BHttpAuthenticationMap* fAuthenticationMap;
			typedef BObjectList<const BCertificate> BCertificateSet;
			BCertificateSet		fCertificates;

			BString				fProxyHost;
			uint16				fProxyPort;
};


#endif // _B_URL_CONTEXT_H_
