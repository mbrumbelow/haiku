/*
 * Copyright 2002-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 * 		Daniel Reinhold, danielre@users.sf.net
 * 		Oliver Tappe, zooey@hirschkaefer.de
 */


#include <PosixLocaleConv.h>

#ifndef _KERNEL_MODE
#include "LocaleBackend.h"

using BPrivate::Libroot::GetCurrentLocaleBackend;
#endif


extern "C" struct lconv*
localeconv(void)
{
#ifndef _KERNEL_MODE
	if (GetCurrentLocaleBackend())
		return const_cast<lconv*>(GetCurrentLocaleBackend()->LocaleConv());
#endif

	return &BPrivate::Libroot::gPosixLocaleConv;
}
