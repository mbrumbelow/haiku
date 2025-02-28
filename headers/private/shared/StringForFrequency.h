/*
 * Copyright 2025 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef STRING_FOR_FREQUENCY_H
#define STRING_FOR_FREQUENCY_H

#include <SupportDefs.h>
#include <String.h>


namespace BPrivate {


BString string_for_frequency(int64 frequencyInMegahertz);


}	// namespace BPrivate


using BPrivate::string_for_frequency;


#endif // STRING_FOR_FREQUENCY_H
