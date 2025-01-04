/*
 * Copyright 2025 Haiku, Inc.
 * Distributed under terms of the MIT license.
 */

#include <unicode/uversion.h>
#include "UnitFormat.h"

#include <ICUWrapper.h>

#include <unicode/measunit.h>
#include <unicode/numberformatter.h>



U_NAMESPACE_USE


BUnitFormat::BUnitFormat(int unitId, const BLocale* locale)
	: BFormat(locale)
{
	MeasureUnit unit;

	switch (unitId) {
		case B_CELSIUS_UNIT:
			unit = MeasureUnit::getCelsius();
			break;
		case B_FAHRENHEIT_UNIT:
			unit = MeasureUnit::getFahrenheit();
			break;
		default:
			fInitStatus = B_BAD_VALUE;
			return;
	}

	fFormatter = new number::LocalizedNumberFormatter(icu::number::NumberFormatter::withLocale(fLanguage.Code()).unit(unit));
}


BUnitFormat::BUnitFormat(int unitId, const BLanguage& language, const BFormattingConventions& conventions)
	: BFormat(language, conventions)
{
	MeasureUnit unit;

	switch (unitId) {
		case B_CELSIUS_UNIT:
			unit = MeasureUnit::getCelsius();
			break;
		case B_FAHRENHEIT_UNIT:
			unit = MeasureUnit::getFahrenheit();
			break;
		default:
			fInitStatus = B_BAD_VALUE;
			return;
	}

	fFormatter = new number::LocalizedNumberFormatter(icu::number::NumberFormatter::withLocale(fLanguage.Code()).unit(unit));
}


BUnitFormat::~BUnitFormat()
{
	delete fFormatter;
}


status_t
BUnitFormat::Format(BString& buffer, const int32 value) const
{
	UErrorCode icuStatus = U_ZERO_ERROR;

	UnicodeString unicodeResult;
	number::FormattedNumber number = fFormatter->formatInt(value, icuStatus);

	if (!U_SUCCESS(icuStatus))
		return B_ERROR;

	BStringByteSink byteSink(&buffer);
	number.toTempString(icuStatus).toUTF8(byteSink);

	if (!U_SUCCESS(icuStatus))
		return B_ERROR;

	return B_OK;
}
