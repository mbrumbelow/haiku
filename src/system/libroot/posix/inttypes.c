/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <inttypes.h>
#include <stdlib.h>


intmax_t
strtoimax(const char *string, char **_end, int base)
{
	return (intmax_t)strtoll(string, _end, base);
}


uintmax_t
strtoumax(const char *string, char **_end, int base)
{
	return (intmax_t)strtoull(string, _end, base);
}

