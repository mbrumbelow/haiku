/*
 * Copyright 2024 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef STRING_FOR_FREQUENCY_H
#define STRING_FOR_FREQUENCY_h

#include <SupportDefs.h>
#include <String.h>


namespace BPrivate {


BString string_for_frequency(int64 frequencyInHertz);


} // namespace BPrivate


using BPrivate::string_for_frequency;


#endif // STRING_FOR_FREQUENCY_H
