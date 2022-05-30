/*
 * Copyright 2022, Trung Nguyen, trungnt282910@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#ifndef _KERNEL_MODE
#include <ctype.h>
#include <pthread.h>

#include <ThreadLocale.h>


// From glibc's localeinfo.h
extern BPrivate::Libroot::LocaleStruct _nl_global_locale;


namespace BPrivate {
namespace Libroot {


static pthread_key_t sLocaleStructPthreadKey;
static pthread_once_t sLocaleStructPthreadKeyOnce = PTHREAD_ONCE_INIT;


static void
DestroyLocaleStruct(void* ptr)
{
	LocaleStruct* localeStruct = (LocaleStruct*)ptr;
	delete localeStruct;
}

static void
MakeLocaleStructKey()
{
	pthread_key_create(&sLocaleStructPthreadKey, DestroyLocaleStruct);
}

static LocaleStruct*
GetCurrentLocaleStruct()
{
	LocaleStruct* ptr;
	pthread_once(&sLocaleStructPthreadKeyOnce, MakeLocaleStructKey);
	if ((ptr = (LocaleStruct*)pthread_getspecific(sLocaleStructPthreadKey)) == NULL) {
		ptr = new LocaleStruct(_nl_global_locale);
		pthread_setspecific(sLocaleStructPthreadKey, ptr);
	}
	return ptr;
}


extern "C" const unsigned short**
__ctype_b_loc()
{
	return &GetCurrentLocaleStruct()->__ctype_b;
}

extern "C" const int**
__ctype_tolower_loc()
{
	return &GetCurrentLocaleStruct()->__ctype_tolower;
}

extern "C" const int**
__ctype_toupper_loc()
{
	return &GetCurrentLocaleStruct()->__ctype_toupper;
}

// Exported so that glibc could also use.
extern "C" LocaleStruct*
_nl_current_locale()
{
	return GetCurrentLocaleStruct();
}


}	// namespace Libroot
}	// namespace BPrivate


#endif //
