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
#include "LocaleInternal.h"


using BPrivate::Libroot::GetLocalesFromEnvironment;
using BPrivate::Libroot::LocaleBackend;
using BPrivate::Libroot::LocaleBackendData;
using BPrivate::Libroot::LocaleDataBridge;


extern "C" locale_t
newlocale(int category_mask, const char* locale, locale_t base)
{
	if (((category_mask | LC_ALL_MASK) != LC_ALL_MASK) || (locale == NULL)) {
		errno = EINVAL;
		return (locale_t)0;
	}

	bool newObject = false;
	LocaleBackendData* localeObject = (LocaleBackendData*)base;

	if (localeObject == NULL) {
		localeObject = new (std::nothrow) LocaleBackendData;
		if (localeObject == NULL) {
			errno = ENOMEM;
			return (locale_t)0;
		}
		localeObject->magic = LOCALE_T_MAGIC;
		localeObject->backend = NULL;
		localeObject->databridge = NULL;

		newObject = true;
	}

	LocaleBackend*& backend = localeObject->backend;
	LocaleDataBridge*& databridge = localeObject->databridge;

	const char* locales[LC_LAST + 1];
	for (int lc = 0; lc <= LC_LAST; lc++)
		locales[lc] = NULL;

	if (*locale == '\0') {
		if (category_mask == LC_ALL_MASK) {
			GetLocalesFromEnvironment(LC_ALL, locales);
		} else {
			for (int lc = 1; lc <= LC_LAST; ++lc) {
				if (category_mask & (1 << (lc - 1))) {
					GetLocalesFromEnvironment(lc, locales);
				}
			}
		}
	} else {
		if (category_mask == LC_ALL_MASK) {
			locales[LC_ALL] = locale;
		}
		for (int lc = 1; lc <= LC_LAST; ++lc) {
			if (category_mask & (1 << (lc - 1))) {
				locales[lc] = locale;
			}
		}
	}

	if (!backend) {
		// for any locale other than POSIX/C, we try to activate the ICU
		// backend
		bool needBackend = false;
		for (int lc = 0; lc <= LC_LAST; lc++) {
			if (locales[lc] != NULL && strcasecmp(locales[lc], "POSIX") != 0
					&& strcasecmp(locales[lc], "C") != 0) {
				needBackend = true;
				break;
			}
		}
		if (needBackend) {
			backend = LocaleBackend::CreateBackend();
			if (backend == NULL) {
				errno = ENOMEM;
				if (newObject) {
					delete localeObject;
				}
				return (locale_t)0;
			}
			databridge = new (std::nothrow) LocaleDataBridge(false);
			if (databridge == NULL) {
				errno = ENOMEM;
				delete backend;
				if (newObject) {
					delete localeObject;
				}
				return (locale_t)0;
			}
			backend->Initialize(databridge);
		}
	}

	BPrivate::ErrnoMaintainer errnoMaintainer;

	if (backend != NULL) {
		for (int lc = 0; lc <= LC_LAST; lc++) {
			if (locales[lc] != NULL) {
				locale = backend->SetLocale(lc, locales[lc]);
				if (lc == LC_ALL) {
					// skip the rest, LC_ALL overrides
					break;
				}
			}
		}
	}

	return (locale_t)localeObject;
}
