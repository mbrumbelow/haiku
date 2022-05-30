/*
 * Copyright 2022, Trung Nguyen, trungnt282910@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <locale.h>

#include "LocaleBackend.h"
#include "LocaleInternal.h"


using BPrivate::Libroot::gGlobalLocaleDataBridge;
using BPrivate::Libroot::gLocaleInfo;
using BPrivate::Libroot::LocaleDataBridge;


extern "C" locale_t
uselocale(locale_t newLoc)
{
    locale_t oldLoc = gLocaleInfo;
    if (gLocaleInfo == NULL) {
        oldLoc = LC_GLOBAL_LOCALE;
    }

    if (newLoc != (locale_t)0) {
        if (newLoc == LC_GLOBAL_LOCALE) {
            gLocaleInfo = NULL;
        } else {
            if (newLoc->magic != LOCALE_T_MAGIC) {
                errno = EINVAL;
                return (locale_t)0;
            }
            gLocaleInfo = newLoc;
        }

        if (gLocaleInfo != NULL) {
            ((LocaleDataBridge*)gLocaleInfo->databridge)->ApplyToCurrentThread();
        } else {
            gGlobalLocaleDataBridge.ApplyToCurrentThread();
        }
    }

    return oldLoc;
}
