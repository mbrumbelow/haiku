/*
 * Copyright 2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "RepositoryUrlUtils.h"


// these are mappings of known legacy identifier URLs that are possibly
// still present in some installations.

static const char* kLegacyUrlMappings[] = {
	"https://eu.hpkg.haiku-os.org/haikuports/master/x86_gcc2/current",
	"https://hpkg.haiku-os.org/haikuports/master/x86_gcc2/current",
	"https://eu.hpkg.haiku-os.org/haikuports/master/x86_64/current",
	"https://hpkg.haiku-os.org/haikuports/master/x86_64/current",
	NULL,
	NULL
};


/*!	See #14927 -- some older legacy URLs (unique identifiers) from early in
	the days of the Haiku package management system changed and so those
	hard-coded cases need to be transformed to work.
*/

/*static*/ void
RepositoryUrlUtils::SwapOutLegacyRepositoryIdentiferUrl(BString& url)
{
	for (int32 i = 0; kLegacyUrlMappings[i] != NULL; i += 2) {
		if (url == kLegacyUrlMappings[i]) {
			url.SetTo(kLegacyUrlMappings[i + 1]);
			return;
		}
	}
}


void
RepositoryUrlUtils::NormalizeUrl(BUrl& url)
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

	BUrl normalizedUrl1(url1);
	NormalizeUrl(normalizedUrl1);
	BUrl normalizedUrl2(url2);
	NormalizeUrl(normalizedUrl2);

	return normalizedUrl1 == normalizedUrl2;
}