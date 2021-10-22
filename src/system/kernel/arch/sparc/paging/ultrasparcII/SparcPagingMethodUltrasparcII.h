/*
 * Copyright 2021, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_SPARC_PAGING_METHOD_ULTRASPARC_II_H
#define KERNEL_ARCH_SPARC_PAGING_METHOD_ULTRASPARC_II_H


#include <SupportDefs.h>

#include <vm/vm_types.h>


struct kernel_args;
struct VMPhysicalPageMapper;
struct VMTranslationMap;


class SparcPagingMethodUltrasparcII {
public:
			status_t			Init(kernel_args* args,
									VMPhysicalPageMapper** _physicalPageMapper);
	
			status_t			MapEarly(kernel_args* args,
									addr_t virtualAddress,
									phys_addr_t physicalAddress,
									uint8 attributes,
									page_num_t (*get_free_page)(kernel_args*));
};


extern SparcPagingMethodUltrasparcII* gSparcPagingMethod;


#endif
