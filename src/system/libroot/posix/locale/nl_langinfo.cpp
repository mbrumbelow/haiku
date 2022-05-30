/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <langinfo.h>

#include "LocaleBackend.h"

#include <PosixLanginfo.h>


using BPrivate::Libroot::gPosixLanginfo;
using BPrivate::Libroot::GetCurrentLocaleBackend;
using BPrivate::Libroot::LocaleBackend;


extern "C" char*
nl_langinfo(nl_item item)
{
	if (item < 0 || item >= _NL_LANGINFO_LAST)
		return const_cast<char*>("");

	if (GetCurrentLocaleBackend() != NULL)
		return const_cast<char*>(GetCurrentLocaleBackend()->GetLanginfo(item));

	return const_cast<char*>(gPosixLanginfo[item]);
}

extern "C" char*
nl_langinfo_l(nl_item item, locale_t locale)
{
	if (item < 0 || item >= _NL_LANGINFO_LAST)
		return const_cast<char*>("");

	return const_cast<char*>(((LocaleBackend*)locale->backend)->GetLanginfo(item));
}
