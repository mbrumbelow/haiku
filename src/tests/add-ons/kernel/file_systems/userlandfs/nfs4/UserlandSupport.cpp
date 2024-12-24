/*
 * Copyright 2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include <OS.h>

#include <heap.h>
#include <util/KernelReferenceable.h>
#include <util/Random.h>


/* Copied from system/kernel/heap.cpp */

DeferredDeletable::~DeferredDeletable()
{
}


/* Modified from original in system/kernel/util/KernelReferenceable */

void
KernelReferenceable::LastReferenceReleased()
{
	delete this;
}


/* Copied from system/kernel/util/Random.cpp */

static uint32	sLast		= 0;

// Taken from "Random number generators: good ones are hard to find",
// Park and Miller, Communications of the ACM, vol. 31, no. 10,
// October 1988, p. 1195.
unsigned int
random_value()
{
	if (sLast == 0)
		sLast = system_time();

	uint32 hi = sLast / 127773;
	uint32 lo = sLast % 127773;

	int32 random = 16807 * lo - 2836 * hi;
	if (random <= 0)
		random += MAX_RANDOM_VALUE;
	sLast = random;
	return random % (MAX_RANDOM_VALUE + 1);
}
