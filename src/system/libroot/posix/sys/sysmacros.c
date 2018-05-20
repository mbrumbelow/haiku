/*
 * Copyright 2018 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

#include <sys/sysmacros.h>

dev_t
haiku_dev_makedev(const unsigned int ma, const unsigned int mi)
{
	return ((ma << 8) & 0xFF00U) | (mi & 0xFFFF00FFU);
}

unsigned int
haiku_dev_major(const dev_t devnum)
{
	return (devnum & 0xFF00U) >> 8;
}

unsigned int
haiku_dev_minor(const dev_t devnum)
{
	return devnum & 0xFFFF00FFU;
}
