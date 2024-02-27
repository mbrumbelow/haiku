/*
 * Copyright 2012-2024, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "StringForRate.h"

#include <stdio.h>

#include <NumberFormat.h>
#include <StringFormat.h>
#include <SystemCatalog.h>

using BPrivate::gSystemCatalog;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "StringForRate"


namespace BPrivate {


const char*
string_for_rate(double rate, char* string, size_t stringSize)
{
	BNumberFormat numberFormat;
	BString printedUsage;

	double value = rate / 1024.0;
	if (value < 1.0) {
		BString tmp;
		BStringFormat format(gSystemCatalog.GetString(
			B_TRANSLATE_MARK("{0, plural, one{# byte/s} other{# bytes/s}}"),
			B_TRANSLATION_CONTEXT,
			"bytes per second"));

		if (numberFormat.Format(printedUsage, (int32)rate) == B_OK) {
			format.Format(printedUsage, (int)rate);
			strlcpy(string, printedUsage.String(), stringSize);
		} else {
			format.Format(tmp, (int)rate);
			strlcpy(string, tmp.String(), stringSize);
		}

		return string;
	}

	const char* kFormats[] = { "%s KiB/s", "%s MiB/s", "%s GiB/s", "%s TiB/s" };
	const char* kRawFormats[] = { "%.2f KiB/s", "%.2f MiB/s", "%.2f GiB/s", "%.2f TiB/s" };

	size_t index = 0;
	while (index < sizeof(kFormats) / sizeof(kFormats[0]) && value < 1.0) {
		value *= 1024.0;
		index++;
	}

	if (numberFormat.SetPrecision(2) == B_OK && numberFormat.Format(printedUsage, value) == B_OK) {
		snprintf(string, stringSize, gSystemCatalog.GetString(B_TRANSLATE_MARK(kFormats[index]),
			B_TRANSLATION_CONTEXT, "unit per second"), printedUsage.String());
	} else {
		snprintf(string, stringSize, gSystemCatalog.GetString(B_TRANSLATE_MARK(kRawFormats[index]),
			B_TRANSLATION_CONTEXT, "unit per second"), value);
	}

	return string;
}

}	// namespace BPrivate

