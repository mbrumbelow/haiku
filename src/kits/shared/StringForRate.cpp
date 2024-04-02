/*
 * Copyright 2012-2024, Haiku Inc. All rights reserved.
 * Copyright 2024, Emir SARI, emir_sari@icloud.com.
 * Distributed under the terms of the MIT License.
 */

#include "StringForRate.h"

#include <stdio.h>

#include <Catalog.h>
#include <NumberFormat.h>
#include <StringFormat.h>
#include <SystemCatalog.h>


using BPrivate::gSystemCatalog;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "StringForRate"


namespace BPrivate {


BString
string_for_rate(double rate)
{
	BString printedRate;
	BString rateString;

	double value = rate / 1024.0;
	if (value < 1.0) {
		BStringFormat format(
			B_TRANSLATE_MARK_ALL("{0, plural, one{# byte/s} other{# bytes/s}}",
			B_TRANSLATION_CONTEXT, "bytes per second"));

		format.Format(printedRate, (int)rate);
		rateString.SetToFormat(gSystemCatalog.GetString(printedRate.String(), B_TRANSLATION_CONTEXT,
			"bytes per second"), printedRate.String());

		return rateString;
	}

	const char* kFormats[] = {
		B_TRANSLATE_MARK_ALL("%s KiB/s", B_TRANSLATION_CONTEXT, "units per second"),
		B_TRANSLATE_MARK_ALL("%s MiB/s", B_TRANSLATION_CONTEXT, "units per second"),
		B_TRANSLATE_MARK_ALL("%s GiB/s", B_TRANSLATION_CONTEXT, "units per second"),
		B_TRANSLATE_MARK_ALL("%s TiB/s", B_TRANSLATION_CONTEXT, "units per second")
	};

	size_t index = 0;
	while (index < B_COUNT_OF(kFormats) && value >= 1024.0) {
		value /= 1024.0;
		index++;
	}

	BNumberFormat numberFormat;
	numberFormat.SetPrecision(2);
	numberFormat.Format(printedRate, value);

	rateString.SetToFormat(
		gSystemCatalog.GetString(kFormats[index], B_TRANSLATION_CONTEXT, "units per second"),
		printedRate.String());

	return rateString;
}


} // namespace BPrivate

