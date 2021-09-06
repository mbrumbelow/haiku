/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */

#ifndef _MACHINE_CPUFUNC_H_
#define	_MACHINE_CPUFUNC_H_

#define	HAVE_INLINE_FLS

static __inline int
fls(int mask)
{
	int bit;
	if (mask == 0)
		return 0;
	for (bit = 1; mask != 1; bit++)
		mask = (unsigned int) mask >> 1;
	return bit;
}

#define	HAVE_INLINE_FFSL

static __inline int
ffsl(long mask)
{
	int bit;

	if (mask == 0)
		return (0);
	for (bit = 1; !(mask & 1); bit++)
		mask = (unsigned long)mask >> 1;
	return (bit);
}

#endif /* !_MACHINE_CPUFUNC_H_ */
