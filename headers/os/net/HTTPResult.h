/*
 * Copyright 2010-2017 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_HTTP_RESULT_H_
#define _B_HTTP_RESULT_H_


#include <iostream>

#include <HttpHeaders.h>
#include <String.h>
#include <Url.h>
#include <UrlResult.h>


class BURLRequest;


class BHTTPResult: public BURLResult {
			friend class 				BHTTPRequest;
			
public:
										BHTTPResult(const BURL& url);
										BHTTPResult(BMessage*);
										BHTTPResult(const BHTTPResult& other);
										~BHTTPResult();

	// Result parameters modifications
			void						SetUrl(const BURL& url);

	// Result parameters access
			const BURL&					Url() const;
			BString						ContentType() const;
			size_t						Length() const;

	// HTTP-Specific stuff
			const BHTTPHeaders&			Headers() const;
			const BString&				StatusText() const;
			int32						StatusCode() const;

	// Result tests
			bool						HasHeaders() const;

	// Overloaded members
			BHTTPResult&				operator=(const BHTTPResult& other);

	virtual	status_t					Archive(BMessage*, bool) const;
	static	BArchivable*				Instantiate(BMessage*);
private:
			BURL						fUrl;
			
			BHTTPHeaders 				fHeaders;
			int32						fStatusCode;
			BString						fStatusString;
};


#endif // _B_URL_RESULT_H_
