/*
 * Copyright 2012-2024, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "StringForRate.h"

#include <stdio.h>

#include <Catalog.h>
#include <NumberFormat.h>
#include <StringFormat.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "StringForRate"


namespace BPrivate {


const char*
string_for_rate(double rate, char* string, size_t stringSize)
{
	BNumberFormat	numberFormat;
	BString			printedRate;

	double value = rate / 1024.0;
	if (value < 1.0) {
		BStringFormat format(B_TRANSLATE_MARK_ALL("{0, plural, one{# byte/s} other{# bytes/s}}",
			"bytes per second", ""));

		format.Format(printedRate, (int)rate);
		strlcpy(string, printedRate.String(), stringSize);

		return string;
	}

	const char* kFormats[] = {
		B_TRANSLATE_MARK_ALL("%s KiB/s", "kilobytes per second", ""),
		B_TRANSLATE_MARK_ALL("%s MiB/s", "megabytes per second", ""),
		B_TRANSLATE_MARK_ALL("%s GiB/s", "gigabytes per second", ""),
		B_TRANSLATE_MARK_ALL("%s TiB/s", "terabytes per second", "")
	};

	size_t index = 0;
	while (index < B_COUNT_OF(kFormats) && value >= 1024.0) {
		value /= 1024.0;
		index++;
	}

	numberFormat.SetPrecision(2);
	numberFormat.Format(printedRate, value);
	snprintf(string, stringSize, kFormats[index], printedRate.String());

	return string;
}


}	// namespace BPrivate

