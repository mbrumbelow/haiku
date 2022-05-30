/*
 * Copyright 2022, Trung Nguyen, trungnt282910@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <locale.h>

#include "LocaleBackend.h"


using BPrivate::Libroot::LocaleBackend;
using BPrivate::Libroot::LocaleDataBridge;


extern "C" void
freelocale(locale_t locobj)
{
    if (locobj->backend) {
        LocaleBackend::DestroyBackend((LocaleBackend*)locobj->backend);
        LocaleDataBridge* databridge = (LocaleDataBridge*)locobj->databridge;
        delete databridge;
    }
    delete locobj;
}
