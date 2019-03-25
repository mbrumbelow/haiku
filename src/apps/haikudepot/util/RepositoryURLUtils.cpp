/*
 * Copyright 2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "RepositoryUrlUtils.h"


void
RepositoryUrlUtils::NormalizeUrl(BURL& url)
{
	if (url.Protocol() == "https")
		url.SetProtocol("http");

	BString path(url.Path());

	if (path.EndsWith("/"))
		url.SetPath(path.Truncate(path.Length() - 1));
}


bool
RepositoryUrlUtils::EqualsNormalized(const BString& url1, const BString& url2)
{
	if (url1.IsEmpty())
		return false;

	BURL normalizedUrl1(url1);
	NormalizeUrl(normalizedUrl1);
	BURL normalizedUrl2(url2);
	NormalizeUrl(normalizedUrl2);

	return normalizedUrl1 == normalizedUrl2;
}