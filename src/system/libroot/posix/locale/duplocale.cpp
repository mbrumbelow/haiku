/*
 * Copyright 2022, Trung Nguyen, trungnt282910@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <new>
#include <errno.h>
#include <locale.h>
#include <strings.h>

#include <ErrnoMaintainer.h>

#include "LocaleBackend.h"


using BPrivate::Libroot::gGlobalLocaleBackend;
using BPrivate::Libroot::LocaleBackend;
using BPrivate::Libroot::LocaleDataBridge;


extern "C" locale_t
duplocale(locale_t locobj)
{
    locale_t newObj = new (std::nothrow) __locale_t;
    if (newObj == NULL) {
        errno = ENOMEM;
        return (locale_t)0;
    }

    LocaleBackend* backend = (locobj == LC_GLOBAL_LOCALE) ?
        gGlobalLocaleBackend : (LocaleBackend*)locobj->backend;

    if (backend == NULL) {
        newObj->backend = NULL;
        return newObj;
    }

    // Check if everything is set to "C" or "POSIX",
    // and avoid making a backend.
    const char* localeDescription = backend->SetLocale(LC_ALL, NULL);

    if ((strcasecmp(localeDescription, "POSIX") == 0) || (strcasecmp(localeDescription, "C") == 0)) {
        newObj->backend = NULL;
        return newObj;
    }

    BPrivate::ErrnoMaintainer errnoMaintainer;

    LocaleBackend*& newBackend = (LocaleBackend*&)newObj->backend;
    LocaleDataBridge*& newDataBridge = (LocaleDataBridge*&)newObj->databridge;
    newBackend = LocaleBackend::CreateBackend();

    if (newBackend == NULL) {
        errno = ENOMEM;
        delete newObj;
        return (locale_t)0;
    }

    newDataBridge = new (std::nothrow) LocaleDataBridge(false);

    if (newDataBridge == NULL) {
        errno = ENOMEM;
        delete newBackend;
        delete newObj;
        return (locale_t)0;
    }

    newBackend->Initialize(newDataBridge);

    // Skipping LC_ALL. Asking for LC_ALL would force the backend
    // to query each other value once, anyway.
    for (int lc = 1; lc <= LC_LAST; ++lc) {
        newBackend->SetLocale(lc, backend->SetLocale(lc, NULL));
    }

    return newObj;
}
