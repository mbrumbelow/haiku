/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef STRING_FOR_RATE_H
#define STRING_FOR_RATE_H

#include <String.h>
#include <SupportDefs.h>


namespace BPrivate {


BString string_for_rate(double rate);


}	// namespace BPrivate


using BPrivate::string_for_rate;


#endif // COLOR_QUANTIZER_H
