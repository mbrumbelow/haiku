/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */


// find first (least significant) set bit
extern "C" int
ffs(int value)
{
	return __builtin_ffs(value);
}
