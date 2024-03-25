/*
 * Copyright 2024 Haiku Inc. All rights reserved.
 * Copyright 2024, Emir SARI, emir_sari@icloud.com.
 * Distributed under the terms of the MIT License.
 */

#include "StringForFrequency.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <NumberFormat.h>
#include <SystemCatalog.h>


using BPrivate::gSystemCatalog;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "StringForFrequency"


namespace BPrivate {


const char*
string_for_frequency(int64 frequencyInHertz, char* string, size_t stringSize)
{
	BString printedFrequency;
	double value = frequencyInHertz * 1000000000.0;

	const char* kFormats[] = {
		B_TRANSLATE_MARK_ALL("%s nHZ", B_TRANSLATION_CONTEXT, "frequency unit"),
		B_TRANSLATE_MARK_ALL("%s µHz", B_TRANSLATION_CONTEXT, "frequency unit"),
		B_TRANSLATE_MARK_ALL("%s mHz", B_TRANSLATION_CONTEXT, "frequency unit"),
		B_TRANSLATE_MARK_ALL("%s Hz", B_TRANSLATION_CONTEXT, "frequency unit"),
		B_TRANSLATE_MARK_ALL("%s KHz", B_TRANSLATION_CONTEXT, "frequency unit"),
		B_TRANSLATE_MARK_ALL("%s MHz", B_TRANSLATION_CONTEXT, "frequency unit"),
		B_TRANSLATE_MARK_ALL("%s GHz", B_TRANSLATION_CONTEXT, "frequency unit"),
		B_TRANSLATE_MARK_ALL("%s THz", B_TRANSLATION_CONTEXT, "frequency unit")
	};

	size_t index = 0;
	while (index < B_COUNT_OF(kFormats) && value >= 1000.0) {
		value /= 1000.0;
		index++;
	}

	BNumberFormat numberFormat;
	if (index >= 6)
		numberFormat.SetPrecision(2);
	else
		numberFormat.SetPrecision(0);
	numberFormat.Format(printedFrequency, value);
	snprintf(string, stringSize, gSystemCatalog.GetString(kFormats[index], B_TRANSLATION_CONTEXT,
		"frequency unit"), printedFrequency.String());

	return string;
}


}	// namespace BPrivate

