/* Convert string for NaN payload to corresponding NaN.  Wide strings.
   Copyright (C) 1997-2025 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#define STRING_TYPE wchar_t
#define L_(Ch) L##Ch
#ifdef __HAIKU__
#include <localeinfo.h>
extern unsigned long long int ____wcstoull_l_internal (const wchar_t *,
						       wchar_t **, int, int,
						       uint8_t, locale_t);
#define STRTOULL(S, E, B) ____wcstoull_l_internal ((S), (E), (B), 0,	\
						   0, __posix_locale_t())
#else
#define STRTOULL(S, E, B) ____wcstoull_l_internal ((S), (E), (B), 0,	\
						   false, _nl_C_locobj_ptr)
#endif
