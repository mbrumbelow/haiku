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


class BHttpResult: public BURLResult {
			friend class 				BHttpRequest;
			
public:
										BHttpResult(const BURL& url);
										BHttpResult(BMessage*);
										BHttpResult(const BHttpResult& other);
										~BHttpResult();

	// Result parameters modifications
			void						SetUrl(const BURL& url);

	// Result parameters access
			const BURL&					Url() const;
			BString						ContentType() const;
			size_t						Length() const;

	// HTTP-Specific stuff
			const BHttpHeaders&			Headers() const;
			const BString&				StatusText() const;
			int32						StatusCode() const;

	// Result tests
			bool						HasHeaders() const;

	// Overloaded members
			BHttpResult&				operator=(const BHttpResult& other);

	virtual	status_t					Archive(BMessage*, bool) const;
	static	BArchivable*				Instantiate(BMessage*);
private:
			BURL						fUrl;
			
			BHttpHeaders 				fHeaders;
			int32						fStatusCode;
			BString						fStatusString;
};


#endif // _B_URL_RESULT_H_
