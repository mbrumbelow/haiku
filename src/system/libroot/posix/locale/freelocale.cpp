/*
 * Copyright 2022, Trung Nguyen, trungnt282910@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <locale.h>

#include "LocaleBackend.h"


using BPrivate::Libroot::LocaleBackend;
using BPrivate::Libroot::LocaleBackendData;
using BPrivate::Libroot::LocaleDataBridge;


extern "C" void
freelocale(locale_t l)
{
    LocaleBackendData* locobj = (LocaleBackendData*)l;
    
    if (locobj->backend) {
        LocaleBackend::DestroyBackend(locobj->backend);
        LocaleDataBridge* databridge = locobj->databridge;
        delete databridge;
    }
    delete locobj;
}
