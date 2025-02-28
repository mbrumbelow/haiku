/*
 * Copyright 2025 Haiku Inc. All rights reserved.
 * Copyright 2025, Emir SARI, emir_sari@icloud.com.
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


BString
string_for_frequency(double frequencyInMegahertz)
{
	BString frequencyString;
	BString printedFrequency;
	double value = frequencyInMegahertz * 1000000.0;

	const char* kFormats[] = {
		B_TRANSLATE_MARK_ALL("%s nHz", B_TRANSLATION_CONTEXT, "frequency unit, use no-break space"),
		B_TRANSLATE_MARK_ALL("%s µHz", B_TRANSLATION_CONTEXT, "frequency unit, use no-break space"),
		B_TRANSLATE_MARK_ALL("%s mHz", B_TRANSLATION_CONTEXT, "frequency unit, use no-break space"),
		B_TRANSLATE_MARK_ALL("%s Hz", B_TRANSLATION_CONTEXT, "frequency unit, use no-break space"),
		B_TRANSLATE_MARK_ALL("%s KHz", B_TRANSLATION_CONTEXT, "frequency unit, use no-break space"),
		B_TRANSLATE_MARK_ALL("%s MHz", B_TRANSLATION_CONTEXT, "frequency unit, use no-break space"),
		B_TRANSLATE_MARK_ALL("%s GHz", B_TRANSLATION_CONTEXT, "frequency unit, use no-break space"),
		B_TRANSLATE_MARK_ALL("%s THz", B_TRANSLATION_CONTEXT, "frequency unit, use no-break space")
	};

	size_t index = 3;
	if (value < 1.0) {
		while (index > 0 && value < 1.0) {
			value *= 1000.0;
			index--;
		}
	} else {
		while (index < B_COUNT_OF(kFormats) - 1 && value >= 1000.0) {
			value /= 1000.0;
			index++;
		}
	}

	BNumberFormat numberFormat;
	numberFormat.SetPrecision(index > 5 ? 2 : 0);

	numberFormat.Format(printedFrequency, value);
	frequencyString.SetToFormat(
		gSystemCatalog.GetString(kFormats[index], B_TRANSLATION_CONTEXT,
		"frequency unit, use no-break space"),
		printedFrequency.String());

	return frequencyString;
}


} // namespace BPrivate
