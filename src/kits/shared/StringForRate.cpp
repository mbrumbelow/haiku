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
	BString trKey;

	double kib = rate / 1024.0;
	double mib = kib / 1024.0;
	double gib = mib / 1024.0;
	double tib = gib / 1024.0;
	status_t precision;
	status_t status;

	if (kib < 1.0) {
		BString tmp;
		BStringFormat format(gSystemCatalog.GetString(
			B_TRANSLATE_MARK("{0, plural, one{# byte/s} other{# bytes/s}}"),
			B_TRANSLATION_CONTEXT,
			"bytes per second"));

		status = numberFormat.Format(printedUsage, (int32)rate);

		if (status == B_OK) {
			format.Format(printedUsage, (int)rate);
			strlcpy(string, printedUsage.String(), stringSize);
		} else {
			format.Format(tmp, (int)rate);
			strlcpy(string, tmp.String(), stringSize);
		}

		return string;
	}

	if (mib < 1.0) {
		precision = numberFormat.SetPrecision(2);
		status = numberFormat.Format(printedUsage, kib);

		if ((status == B_OK) && (precision == B_OK)) {
			trKey = B_TRANSLATE_MARK("%s KiB/s");
			snprintf(string, stringSize, gSystemCatalog.GetString(trKey, B_TRANSLATION_CONTEXT,
				"KiB per second"), printedUsage.String());
		} else {
			trKey = B_TRANSLATE_MARK("%3.2f KiB/s");
			snprintf(string, stringSize, gSystemCatalog.GetString(trKey, B_TRANSLATION_CONTEXT,
				"KiB per second"), kib);
		}

		return string;
	}

	if (gib < 1.0) {
		precision = numberFormat.SetPrecision(2);
		status = numberFormat.Format(printedUsage, mib);

		if ((status == B_OK) && (precision == B_OK)) {
			trKey = B_TRANSLATE_MARK("%s MiB/s");
			snprintf(string, stringSize, gSystemCatalog.GetString(trKey, B_TRANSLATION_CONTEXT,
				"MiB per second"), printedUsage.String());
		} else {
			trKey = B_TRANSLATE_MARK("%3.2f MiB/s");
			snprintf(string, stringSize, gSystemCatalog.GetString(trKey, B_TRANSLATION_CONTEXT,
				"MiB per second"), mib);
		}

		return string;
	}

	if (tib < 1.0) {
		precision = numberFormat.SetPrecision(2);
		status = numberFormat.Format(printedUsage, gib);

		if ((status == B_OK) && (precision == B_OK)) {
			trKey = B_TRANSLATE_MARK("%s GiB/s");
			snprintf(string, stringSize, gSystemCatalog.GetString(trKey, B_TRANSLATION_CONTEXT,
				"GiB per second"), printedUsage.String());
		} else {
			trKey = B_TRANSLATE_MARK("%3.2f GiB/s");
			snprintf(string, stringSize, gSystemCatalog.GetString(trKey, B_TRANSLATION_CONTEXT,
				"GiB per second"), gib);
		}

		return string;
	}

	precision = numberFormat.SetPrecision(3);
	status = numberFormat.Format(printedUsage, tib);

	if ((status == B_OK) && (precision == B_OK)) {
		trKey = B_TRANSLATE_MARK("%s TiB/s");
		snprintf(string, stringSize, gSystemCatalog.GetString(trKey, B_TRANSLATION_CONTEXT,
			"TiB per second"), printedUsage.String());
	} else {
		trKey = B_TRANSLATE_MARK("%.3f TiB/s");
		snprintf(string, stringSize, gSystemCatalog.GetString(trKey, B_TRANSLATION_CONTEXT,
			"TiB per second"), tib);
	}

	return string;
}

}	// namespace BPrivate

