/*
 * Copyright 2010-2020 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */

#include <UrlProtocolRoster.h>

#include <new>

#include <DataRequest.h>
#include <Debug.h>
#include <FileRequest.h>
#include <FindDirectory.h>
#include <HttpRequest.h>
#include <UrlRequest.h>


/* static */ BUrlRequest*
BUrlProtocolRoster::MakeRequest(const BUrl& url,
	BUrlProtocolListener* listener, BUrlContext* context)
{
	if (url.Protocol() == "http") {
		return new(std::nothrow) BHttpRequest(url, false, "HTTP", listener,
			context);
	} else if (url.Protocol() == "https") {
		return new(std::nothrow) BHttpRequest(url, true, "HTTPS", listener,
			context);
	} else if (url.Protocol() == "file") {
		return new(std::nothrow) BFileRequest(url, listener, context);
	} else if (url.Protocol() == "data") {
		return new(std::nothrow) BDataRequest(url, listener, context);
	}

	// If it's not one of the core protocol handlers, check add-ons
	return BUrlProtocolRoster::_ScanAddOnProtocols(url, listener, context);
}


/* static */ create_uri_addon*
BUrlProtocolRoster::_LoadAddOnProtocol(BPath path)
{
        image_id image = load_add_on(path.Path());
	if (image < 0) {
		//B_BAD_IMAGE_ID;
		return NULL;
	}
	create_uri_addon* createFunc;
	if (get_image_symbol(image, "instantiate_uri_addon", B_SYMBOL_TYPE_TEXT,
		(void**)&createFunc) != B_OK) {
		unload_add_on(image);
		//B_MISSING_SYMBOL;
		return NULL;
	}
	return createFunc;
}


/* static */ BUrlRequest*
BUrlProtocolRoster::_ScanAddOnProtocols(const BUrl& url,
	BUrlProtocolListener* listener, BUrlContext* context)
{
	status_t result;

	BPath userNonPackagedPath;
	result = find_directory(B_USER_NONPACKAGED_ADDONS_DIRECTORY,
		&userNonPackagedPath);
	if (result == B_OK) {
		userNonPackagedPath.Append("network/uri_handlers");
		userNonPackagedPath.Append(url.Protocol());
		create_uri_addon* createFunc
			= BUrlProtocolRoster::_LoadAddOnProtocol(userNonPackagedPath);
		if (createFunc != NULL)
			return createFunc(url, listener, context);
	}

	BPath systemNonPackagedPath;
	result = find_directory(B_SYSTEM_NONPACKAGED_ADDONS_DIRECTORY,
		&systemNonPackagedPath);
	if (result == B_OK) {
		systemNonPackagedPath.Append("network/uri_handlers");
		systemNonPackagedPath.Append(url.Protocol());
		create_uri_addon* createFunc
			= BUrlProtocolRoster::_LoadAddOnProtocol(systemNonPackagedPath);
		if (createFunc != NULL)
			return createFunc(url, listener, context);
	}

	BPath systemPath;
	result = find_directory(B_SYSTEM_ADDONS_DIRECTORY, &systemPath);
	if (result == B_OK) {
		systemPath.Append("network/uri_handlers");
		systemPath.Append(url.Protocol());
		create_uri_addon* createFunc
			= BUrlProtocolRoster::_LoadAddOnProtocol(systemPath);
		if (createFunc != NULL)
			return createFunc(url, listener, context);
	}

	return NULL;
}
