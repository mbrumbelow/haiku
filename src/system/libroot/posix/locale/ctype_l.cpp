/*
 * Copyright 2022, Trung Nguyen, trungnt282910@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <ctype.h>
#include <locale.h>

#include <LocaleBackend.h>


#define CTYPE_B(locale) \
(*(((BPrivate::Libroot::LocaleDataBridge*)(locale)->databridge)\
	->ctypeDataBridge.addrOfClassInfoTable))

#define CTYPE_TOUPPER(locale) \
(*(((BPrivate::Libroot::LocaleDataBridge*)(locale)->databridge)\
	->ctypeDataBridge.addrOfToUpperTable))

#define CTYPE_TOLOWER(locale) \
(*(((BPrivate::Libroot::LocaleDataBridge*)(locale)->databridge)\
	->ctypeDataBridge.addrOfToLowerTable))


extern "C"
{


int
isalnum_l(int c, locale_t l)
{
	if (c >= -128 && c < 256)
		return CTYPE_B(l)[c] & (_ISupper | _ISlower | _ISdigit);

	return 0;
}


int
isalpha_l(int c, locale_t l)
{
	if (c >= -128 && c < 256)
		return CTYPE_B(l)[c] & (_ISupper | _ISlower);

	return 0;
}


int
isblank_l(int c, locale_t l)
{
	if (c >= -128 && c < 256)
		return CTYPE_B(l)[c] & _ISblank;

	return 0;
}


int
iscntrl_l(int c, locale_t l)
{
	if (c >= -128 && c < 256)
		return CTYPE_B(l)[c] & _IScntrl;

	return 0;
}


int
isdigit_l(int c, locale_t l)
{
	if (c >= -128 && c < 256)
		return CTYPE_B(l)[c] & _ISdigit;

	return 0;
}


int
isgraph_l(int c, locale_t l)
{
	if (c >= -128 && c < 256)
		return CTYPE_B(l)[c] & _ISgraph;

	return 0;
}


int
islower_l(int c, locale_t l)
{
	if (c >= -128 && c < 256)
		return CTYPE_B(l)[c] & _ISlower;

	return 0;
}


int
isprint_l(int c, locale_t l)
{
	if (c >= -128 && c < 256)
		return CTYPE_B(l)[c] & _ISprint;

	return 0;
}


int
ispunct_l(int c, locale_t l)
{
	if (c >= -128 && c < 256)
		return CTYPE_B(l)[c] & _ISpunct;

	return 0;
}


int
isspace_l(int c, locale_t l)
{
	if (c >= -128 && c < 256)
		return CTYPE_B(l)[c] & _ISspace;

	return 0;
}


int
isupper_l(int c, locale_t l)
{
	if (c >= -128 && c < 256)
		return CTYPE_B(l)[c] & _ISupper;

	return 0;
}


int
isxdigit_l(int c, locale_t l)
{
	if (c >= -128 && c < 256)
		return CTYPE_B(l)[c] & _ISxdigit;

	return 0;
}


int
tolower_l(int c, locale_t l)
{
	if (c >= -128 && c < 256)
		return CTYPE_TOLOWER(l)[c];

	return c;
}


int
toupper_l(int c, locale_t l)
{
	if (c >= -128 && c < 256)
		return CTYPE_TOUPPER(l)[c];

	return c;
}


}
