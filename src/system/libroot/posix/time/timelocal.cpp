/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "timelocal.h"

#include <stddef.h>

#include "LocaleBackend.h"
#include "PosixLCTimeInfo.h"


extern "C" const lc_time_t*
__get_current_time_locale(void)
{
	if (GET_LOCALE_BACKEND())
		return GET_LOCALE_BACKEND()->LCTimeInfo();

	return &BPrivate::Libroot::gPosixLCTimeInfo;
}
