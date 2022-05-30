/*
 * Copyright 2022, Trung Nguyen, trungnt282910@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#ifndef _KERNEL_MODE
#include <ctype.h>


// These functions are intended for scenarios where we cannot 
// link to the whole libroot and access pthread functions; 
// for example, when we're in the static "libruntime_loader.a"

extern "C" const unsigned short** 
__ctype_b_loc()
{
	return &__ctype_b;
}

extern "C" const int**
__ctype_tolower_loc()
{
	return &__ctype_tolower;
}

extern "C" const int**
__ctype_toupper_loc()
{
	return &__ctype_toupper;
}


#endif //_KERNEL_MODE
