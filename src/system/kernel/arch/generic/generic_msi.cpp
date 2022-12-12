/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <arch/generic/msi.h>


MsiDriver* sMsiDriver;


void
msi_set_driver(MsiDriver* driver)
{
	sMsiDriver = driver;
}


bool
msi_supported()
{
	return sMsiDriver != NULL;
}


status_t
msi_allocate_vectors(uint8 count, uint8 *startVector, uint64 *address, uint16 *data)
{
	return sMsiDriver->AllocateVectors(count, *startVector, *address, *data);
}


void
msi_free_vectors(uint8 count, uint8 startVector)
{
	sMsiDriver->FreeVectors(count, startVector);
}

