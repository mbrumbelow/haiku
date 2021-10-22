/*
 * Copyright 2021, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under the terms of the MIT License.
 */


#include "paging/ultrasparcII/SparcPagingMethodUltrasparcII.h"

#include <stdlib.h>
#include <string.h>

#include <boot/kernel_args.h>


//#define TRACE_SPARC_PAGING_METHOD_ULTRASPARC_II
#ifdef TRACE_SPARC_PAGING_METHOD_ULTRASPARC_II
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


SparcPagingMethodUltrasparcII* gSparcPagingMethod;


status_t
SparcPagingMethodUltrasparcII::Init(kernel_args* args,
	VMPhysicalPageMapper** _physicalPageMapper)
{
	memset(fTranslationStorageBuffer, 0, sizeof(fTranslationStorageBuffer));

	// TODO probably some more things needed:
	// - Pre fill the TSB with the data from OpenFirmware
	// - Set up the MMU exception handler to handle MMU faults and feed the MMU°
	return B_OK;
}


status_t
SparcPagingMethodUltrasparcII::MapEarly(kernel_args* args, addr_t virtualAddress,
	phys_addr_t physicalAddress, uint8 attributes,
	page_num_t (*get_free_page)(kernel_args*))
{
	TRACE("SparcPagingMethodUltraasparcII::MapEarly(%#" B_PRIxADDR ", %#" B_PRIxPHYSADDR
		", %#" B_PRIx8 ")\n", virtualAddress, physicalAddress, attributes);

	// Compute the index in the TSB
	uint32 tsbIndex = (virtualAddress >> 13) & 0xFFFF;

	TranslationTableEntry* tableEntry = &fTranslationStorageBuffer[tsbIndex];
	tableEntry->tag = virtualAddress >> 22;
	tableEntry->valid = 1;
	tableEntry->physicalAddress = physicalAddress >> 13;
	tableEntry->writable = attributes & B_KERNEL_WRITE_AREA;
	// TODO figure out if it needs to be executable, and if so, put it in the
	// iMMU in addition to the dMMU? Store that info in one of the software
	// bits? Later implementations of the table entry gained an executable bit.
	return B_NOT_SUPPORTED;
}
