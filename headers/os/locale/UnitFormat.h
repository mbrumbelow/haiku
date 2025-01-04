/*
 * Copyright 2025 Haiku, Inc.
 * Distributed under terms of the MIT license.
 */

#ifndef _B_UNITFORMAT_H_
#define _B_UNITFORMAT_H_


#include <Format.h>


#ifndef U_ICU_NAMESPACE
  #define U_ICU_NAMESPACE icu
#endif
namespace U_ICU_NAMESPACE {
	namespace number {
		class LocalizedNumberFormatter;
	}
}


enum {
	B_CELSIUS_UNIT,
	B_FAHRENHEIT_UNIT
};


class BUnitFormat : public BFormat {

public:
				BUnitFormat(int unit, const BLocale* locale = NULL);
				BUnitFormat(int unit, const BLanguage& language,
					const BFormattingConventions& conventions);

				~BUnitFormat();

	status_t	Format(BString& buffer, const int32 value) const;

private:
	U_ICU_NAMESPACE::number::LocalizedNumberFormatter* fFormatter;
};


#endif /* !_B_UNITFORMAT_H_ */
