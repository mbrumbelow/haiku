/* Convert string for NaN payload to corresponding NaN.  Narrow strings.
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

#define STRING_TYPE char
#define L_(Ch) Ch
#ifdef __HAIKU__
#include <localeinfo.h>
unsigned long long __strtoull_l(const char * nptr, char ** endptr, int base, locale_t locale);
#define STRTOULL(S, E, B) __strtoull_l((S), (E), (B), __posix_locale_t())
#else
#define STRTOULL(S, E, B) ____strtoull_l_internal ((S), (E), (B), 0,	\
						   false, _nl_C_locobj_ptr)
#endif
