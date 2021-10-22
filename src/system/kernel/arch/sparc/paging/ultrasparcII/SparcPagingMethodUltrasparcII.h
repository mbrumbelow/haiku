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


struct TranslationTableEntry {
	union {
		struct {
			uint64 tagGlobal: 1;
			uint64 reserved1: 2;
			uint64 context: 13;
			uint64 reserved2: 6;
			uint64 virtualAddress: 42;
				// The tag contains bits 63-22 of the virtual address. For a 8K
				// page, bits 13-0 define the offset in the page. This means
				// bits 21-14 are not stored anywhere in this structure. They
				// correspond to the offset in the TSB, which explains why the
				// TSB has a minimal size of 512 (mapping 4MB). Using a larger
				// TSB means more bits from the virtual address can be used as
				// a key into it. The largest size allows to cache up to 512MB
				// of address space, which is convenient at early boot because
				// we don't need any other structure to store things then. It
				// is however not large enough to hold the complete address
				// space for a team, and a bit wasteful if we have a separate
				// TSB for each team. For now let's have a single shared TSB
				// (using the context field to store the team ID to make sure
				// we don't mix up mappings from different threads).
		};
		uint64 tag;
	};

	union {
		struct {
			uint64 valid: 1;
			uint64 size: 2;
			uint64 noFaultOnly: 1;
			uint64 invertEndianness: 1;
			uint64 software2: 9;
			uint64 diag: 9;
			uint64 physicalAddress: 28;
			uint64 software1: 6;
			uint64 lock: 1;
			uint64 physicalCacheable: 1;
			uint64 virtualCacheable: 1;
			uint64 sideEffects: 1;
			uint64 privileged: 1;
			uint64 writable: 1;
			uint64 dataGlobal: 1;
		};
		uint64 data;
	};
};


class SparcPagingMethodUltrasparcII {
public:
			status_t			Init(kernel_args* args,
									VMPhysicalPageMapper** _physicalPageMapper);
	
			status_t			MapEarly(kernel_args* args,
									addr_t virtualAddress,
									phys_addr_t physicalAddress,
									uint8 attributes,
									page_num_t (*get_free_page)(kernel_args*));

private:
			TranslationTableEntry fTranslationStorageBuffer[65536];
				// Use the largest possible TTE, which needs 512K of memory.
				// This makes MMU misses be solved as often as possible by
				// finding things in the TTE instead of slower lookups in
				// software-defined mapping structures.
};


extern SparcPagingMethodUltrasparcII* gSparcPagingMethod;


#endif
