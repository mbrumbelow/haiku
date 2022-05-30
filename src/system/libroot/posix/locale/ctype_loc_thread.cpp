/*
 * Copyright 2022, Trung Nguyen, trungnt282910@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#ifndef _KERNEL_MODE
#include <ctype.h>
#include <pthread.h>


namespace BPrivate {
namespace Libroot {


struct ctype_info {
	const unsigned short*	__ctype_b;
	const int* 				__ctype_tolower;
	const int*				__ctype_toupper;
};


static pthread_key_t sCtypeInfoPthreadKey;
static pthread_once_t sCtypeInfoPthreadKeyOnce = PTHREAD_ONCE_INIT;


static void
ctype_info_destroy(void* ptr)
{
	ctype_info* info = (ctype_info*)ptr;
	delete info;
}

static void 
ctype_info_make_key()
{
	pthread_key_create(&sCtypeInfoPthreadKey, ctype_info_destroy);
}

static ctype_info* 
__ctype_info_get()
{
	ctype_info* ptr;
	pthread_once(&sCtypeInfoPthreadKeyOnce, ctype_info_make_key);
	if ((ptr = (ctype_info*)pthread_getspecific(sCtypeInfoPthreadKey)) == NULL) {
		ptr = new ctype_info;
		ptr->__ctype_b = __ctype_b;
		ptr->__ctype_tolower = __ctype_tolower;
		ptr->__ctype_toupper = __ctype_toupper;
		pthread_setspecific(sCtypeInfoPthreadKey, ptr);
	}
	return ptr;
}


extern "C" const unsigned short** 
__ctype_b_loc()
{
	return &__ctype_info_get()->__ctype_b;
}

extern "C" const int**
__ctype_tolower_loc()
{
	return &__ctype_info_get()->__ctype_tolower;
}

extern "C" const int**
__ctype_toupper_loc()
{
	return &__ctype_info_get()->__ctype_toupper;
}


}	// namespace Libroot
}	// namespace BPrivate


#endif //
