/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */


// find first (least significant) set bit
extern "C" int
ffs(int value)
{
#if __GNUC__ >= 3
	return __builtin_ffs(value);
#else
	if (!value)
		return 0;

	asm("rep; bsf %1,%0"
		: "=r" (value)
		: "rm" (value));
	return value;
#endif
}
