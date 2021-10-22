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
	return B_NOT_SUPPORTED;
}


status_t
SparcPagingMethodUltrasparcII::MapEarly(kernel_args* args, addr_t virtualAddress,
	phys_addr_t physicalAddress, uint8 attributes,
	page_num_t (*get_free_page)(kernel_args*))
{
	TRACE("X86PagingMethod64Bit::MapEarly(%#" B_PRIxADDR ", %#" B_PRIxPHYSADDR
		", %#" B_PRIx8 ")\n", virtualAddress, physicalAddress, attributes);

	return B_NOT_SUPPORTED;
}
