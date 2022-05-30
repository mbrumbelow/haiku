/*
 * Copyright 2022, Trung Nguyen, trungnt282910@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <locale.h>

#include "LocaleBackend.h"
#include "LocaleInternal.h"


using BPrivate::Libroot::gGlobalLocaleDataBridge;
using BPrivate::Libroot::GetCurrentLocaleInfo;
using BPrivate::Libroot::LocaleDataBridge;
using BPrivate::Libroot::SetCurrentLocaleInfo;


extern "C" locale_t
uselocale(locale_t newLoc)
{
    locale_t oldLoc = GetCurrentLocaleInfo();
    if (oldLoc == NULL) {
        oldLoc = LC_GLOBAL_LOCALE;
    }

    if (newLoc != (locale_t)0) {
        // Avoid expensive TLS reads with a local variable.
        locale_t appliedLoc = oldLoc;

        if (newLoc == LC_GLOBAL_LOCALE) {
            appliedLoc = NULL;
        } else {
            if (newLoc->magic != LOCALE_T_MAGIC) {
                errno = EINVAL;
                return (locale_t)0;
            }
            appliedLoc = newLoc;
        }

        SetCurrentLocaleInfo(appliedLoc);

        if (appliedLoc != NULL) {
            ((LocaleDataBridge*)appliedLoc->databridge)->ApplyToCurrentThread();
        } else {
            gGlobalLocaleDataBridge.ApplyToCurrentThread();
        }
    }

    return oldLoc;
}
