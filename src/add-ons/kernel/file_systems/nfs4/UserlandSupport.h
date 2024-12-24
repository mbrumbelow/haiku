/*
 * Copyright 2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef NFS4_USERLAND_SUPPORT_H
#define NFS4_USERLAND_SUPPORT_H


static const int	kRandomShift			= 31;


/* Modified from original in Random.h */
template<typename T>
T
get_random()
{
	size_t shift = 0;
	T random = 0;
	while (shift < sizeof(T) * 8) {
		random |= (T)std::rand() << shift;
		shift += kRandomShift;
	}

	return random;
}


#endif	// NFS4_USERLAND_SUPPORT_H
