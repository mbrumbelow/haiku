/*
 * Copyright 2022 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#include "VMSAv8TranslationMap.h"

#include <util/AutoLock.h>
#include <util/ThreadAutoLock.h>
#include <slab/Slab.h>
#include <vm/vm_page.h>
#include <vm/vm_priv.h>
#include <vm/VMCache.h>


static constexpr uint64_t kPteAddrMask = (((1UL << 36) - 1) << 12);
static constexpr uint64_t kPteTLBCompatMask = (kPteAddrMask | (0x3 << 2) | (0x3 << 8));
static constexpr uint64_t kPteAttrMask = ~(kPteAddrMask | 0x3);

static constexpr uint64_t kAttrSWDIRTY = (1UL << 56);
static constexpr uint64_t kAttrSWDBM = (1UL << 55);
static constexpr uint64_t kAttrUXN = (1UL << 54);
static constexpr uint64_t kAttrPXN = (1UL << 53);
static constexpr uint64_t kAttrDBM = (1UL << 51);
static constexpr uint64_t kAttrNG = (1UL << 11);
static constexpr uint64_t kAttrAF = (1UL << 10);
static constexpr uint64_t kAttrSH1 = (1UL << 9);
static constexpr uint64_t kAttrSH0 = (1UL << 8);
static constexpr uint64_t kAttrAP2 = (1UL << 7);
static constexpr uint64_t kAttrAP1 = (1UL << 6);

static constexpr uint64_t kTLBIMask = ((1UL << 44) - 1);

uint32_t VMSAv8TranslationMap::fHwFeature;
uint64_t VMSAv8TranslationMap::fMair;
VMSAv8TranslationMap* VMSAv8TranslationMap::fAsidMapping[256];
spinlock VMSAv8TranslationMap::fAsidLock;
phys_addr_t VMSAv8TranslationMap::fEmptyTable;


void VMSAv8TranslationMap::Init()
{
	vm_page_reservation reservation;
	vm_page_reserve_pages(&reservation, 1, VM_PRIORITY_SYSTEM);
	vm_page* page = vm_page_allocate_page(&reservation, PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR);
	DEBUG_PAGE_ACCESS_END(page);
	fEmptyTable = page->physical_page_number << 12;
	WRITE_SPECIALREG(TTBR0_EL1, fEmptyTable);
	arch_cpu_global_TLB_invalidate();
}


VMSAv8TranslationMap::VMSAv8TranslationMap(
	bool kernel, phys_addr_t pageTable, int pageBits, int vaBits, int minBlockLevel)
	:
	fIsKernel(kernel),
	fPageTable(pageTable),
	fPageBits(pageBits),
	fVaBits(vaBits),
	fMinBlockLevel(minBlockLevel),
	fAsid(-1),
	fRefcount(0)
{
	fInitialLevel = CalcStartLevel(fVaBits, fPageBits);
}


VMSAv8TranslationMap::~VMSAv8TranslationMap()
{
	ASSERT(!fIsKernel);
	ASSERT(fRefcount == 0);

	{
		ThreadCPUPinner pinner(thread_get_current_thread());
		FreeTable(fPageTable, 0, fInitialLevel, [](int level, uint64_t oldPte) {});
	}

	{
		InterruptsLocker irq_locker;
		SpinLocker locker(fAsidLock);

		if (fAsid != -1)
			fAsidMapping[fAsid] = NULL;
	}
}


int
VMSAv8TranslationMap::CalcStartLevel(int vaBits, int pageBits)
{
	int level = 4;

	int bitsLeft = vaBits - pageBits;
	while (bitsLeft > 0) {
		int tableBits = pageBits - 3;
		bitsLeft -= tableBits;
		level--;
	}

	ASSERT(level >= 0);

	return level;
}


// Switch user map into TTBR0.
// Passing kernel map here configures empty page table.
void
VMSAv8TranslationMap::SwitchUserMap(VMSAv8TranslationMap *from, VMSAv8TranslationMap *to)
{
	SpinLocker locker(fAsidLock);

	if (!from->fIsKernel) {
		from->fRefcount--;
	}

	if (!to->fIsKernel) {
		to->fRefcount++;
	} else {
		WRITE_SPECIALREG(TTBR0_EL1, fEmptyTable);
		asm("isb");
		return;
	}

	ASSERT(to->fPageTable != 0);
	uint64_t cpuMask = 1UL << smp_get_current_cpu();
	uint64_t ttbr = to->fPageTable | ((fHwFeature & HW_CNP) != 0 ? 1 : 0);

	if (to->fAsid != -1) {
		WRITE_SPECIALREG(TTBR0_EL1, ((uint64_t)to->fAsid << 48) | ttbr);
		asm("isb");
		return;
	}

	for (size_t i = 0; i < sizeof(fAsidMapping) / sizeof(fAsidMapping[0]); i++) {
		if (fAsidMapping[i] == NULL) {
			WRITE_SPECIALREG(TTBR0_EL1, (i << 48) | ttbr);
			asm("isb");

			to->fAsid = i;
			fAsidMapping[i] = to;
			return;
		}
	}

	for (size_t i = 0; i < sizeof(fAsidMapping) / sizeof(fAsidMapping[0]); i++) {
		if (fAsidMapping[i]->fRefcount == 0) {
			WRITE_SPECIALREG(TTBR0_EL1, (i << 48) | ttbr);

			asm("dsb ishst");
			asm("tlbi aside1is, %0" :: "r" (i << 48));
			asm("dsb ish");
			asm("isb");
			fAsidMapping[i]->fAsid = -1;

			to->fAsid = i;
			fAsidMapping[i] = to;
			return;
		}
	}

	panic("cannot assign ASID");
}


bool
VMSAv8TranslationMap::Lock()
{
	recursive_lock_lock(&fLock);
	return true;
}


void
VMSAv8TranslationMap::Unlock()
{
	recursive_lock_unlock(&fLock);
}


addr_t
VMSAv8TranslationMap::MappedSize() const
{
	panic("VMSAv8TranslationMap::MappedSize not implemented");
	return 0;
}


size_t
VMSAv8TranslationMap::MaxPagesNeededToMap(addr_t start, addr_t end) const
{
	size_t result = 0;
	size_t size = end - start + 1;

	for (int i = fInitialLevel; i < 3; i++) {
		int tableBits = fPageBits - 3;
		int shift = tableBits * (3 - i) + fPageBits;
		uint64_t entrySize = 1UL << shift;

		result += size / entrySize + 2;
	}

	return result;
}


uint64_t*
VMSAv8TranslationMap::TableFromPa(phys_addr_t pa)
{
	// Access page table through linear memory mapping
	return reinterpret_cast<uint64_t*>(KERNEL_PMAP_BASE + pa);
}


uint64_t
VMSAv8TranslationMap::MakeBlock(phys_addr_t pa, int level, uint64_t attr)
{
	ASSERT(level >= fMinBlockLevel && level < 4);

	// Block mappings at upper levels have different encoding
	return pa | attr | (level == 3 ? 0x3 : 0x1);
}


template <typename EntryRemoved>
void
VMSAv8TranslationMap::FreeTable(phys_addr_t ptPa, uint64_t va, int level, EntryRemoved &&entryRemoved)
{
	ASSERT(level < 4);

	int tableBits = fPageBits - 3;
	uint64_t tableSize = 1UL << tableBits;
	uint64_t vaMask = (1UL << fVaBits) - 1;

	int shift = tableBits * (3 - level) + fPageBits;
	uint64_t entrySize = 1UL << shift;

	uint64_t nextVa = va;
	uint64_t* pt = TableFromPa(ptPa);
	for (uint64_t i = 0; i < tableSize; i++) {
		uint64_t oldPte = (uint64_t) atomic_get_and_set64((int64*) &pt[i], 0);

		if (level < 3 && (oldPte & 0x3) == 0x3) {
			FreeTable(oldPte & kPteAddrMask, nextVa, level + 1, entryRemoved);
		} else if ((oldPte & 0x1) != 0) {
			uint64_t fullVa = (fIsKernel ? ~vaMask : 0) | nextVa;
			asm("dsb ishst");
			asm("tlbi vaae1is, %0" :: "r" ((fullVa >> 12) & kTLBIMask));
			// Does it correctly flush block entries at level < 3? We don't use them anyway though.
			// TODO: Flush only currently used ASID (using vae1is)
			entryRemoved(level, oldPte);
		}

		nextVa += entrySize;
	}

	asm("dsb ish");

	vm_page* page = vm_lookup_page(ptPa >> fPageBits);
	DEBUG_PAGE_ACCESS_START(page);
	vm_page_set_state(page, PAGE_STATE_FREE);
}

// Return existing table at given index or create new table there
phys_addr_t
VMSAv8TranslationMap::MakeTable(
	phys_addr_t ptPa, int level, int index, vm_page_reservation* reservation)
{
	ASSERT(level < 3);

	uint64_t* pte = &TableFromPa(ptPa)[index];

	uint64_t oldPte = atomic_get64((int64*) pte);

	int type = oldPte & 0x3;
	if (type == 0x3) {
		// This is table entry already, just return it
		return oldPte & kPteAddrMask;
	} else if (reservation != NULL) {
		// Create new table there
		vm_page* page = vm_page_allocate_page(reservation, PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR);
		phys_addr_t newTablePa = page->physical_page_number << fPageBits;
		DEBUG_PAGE_ACCESS_END(page);

		// We only create mappings at the final level so we don't need to handle
		// splitting block mappings
		ASSERT(type != 0x1);

		// Ensure that writes to page being attached have completed
		asm("dsb ishst");

		// We never replace block mapping so atomic swap is not needed here
		atomic_set64((int64*) pte, newTablePa | 0x3);

		return newTablePa;
	}

	// There's no existing table and we have no reservation
	return 0;
}

template <typename MakePte, typename EntryRemoved>
void
VMSAv8TranslationMap::ProcessRange(phys_addr_t ptPa, int level, addr_t va, phys_addr_t pa, size_t size,
	vm_page_reservation* reservation, VMAction action, MakePte &&makePte, EntryRemoved &&entryRemoved)
{
	ASSERT(level < 4);
	ASSERT(ptPa != 0);
	if (action == VMAction::MAP)
		ASSERT(reservation != NULL);
	else
		ASSERT(reservation == NULL);

	int tableBits = fPageBits - 3;
	uint64_t tableMask = (1UL << tableBits) - 1;

	int shift = tableBits * (3 - level) + fPageBits;
	uint64_t entrySize = 1UL << shift;

	uint64_t entryMask = entrySize - 1;
	uint64_t nextVa = va;
	uint64_t end = va + size;
	int index;

	// Handle misaligned header that straddles entry boundary in next-level table
	if ((va & entryMask) != 0) {
		uint64_t aligned = (va & ~entryMask) + entrySize;
		if (end > aligned) {
			index = (va >> shift) & tableMask;
			phys_addr_t table = MakeTable(ptPa, level, index, reservation);
			if (table != 0) // Skip if there's actually no table there
				ProcessRange(table, level + 1, va, pa, aligned - va, reservation, action, makePte, entryRemoved);
			nextVa = aligned;
		}
	}

	// Handle fully aligned and appropriately sized chunks
	while (nextVa + entrySize <= end) {
		phys_addr_t targetPa = pa + (nextVa - va);
		index = (nextVa >> shift) & tableMask;

		// Only allow mapping in the final level (would require huge page support otherwise)
		bool blockAllowed = (level == 3 || action == VMAction::UNMAP);

		if (blockAllowed) {
			// Everything is aligned, we can make block mapping there
			uint64_t* pte = &TableFromPa(ptPa)[index];

		retry:
			uint64_t oldPte = atomic_get64((int64*) pte);

			if (action != VMAction::MAP && (oldPte & 0x1) == 0) {
				// Skip if there's actually no entry there
				nextVa += entrySize;
				continue;
			}

			uint64_t newPte = makePte(level, nextVa, targetPa, oldPte);

			uint64_t vaMask = (1UL << fVaBits) - 1;
			uint64_t fullVa = (fIsKernel ? ~vaMask : 0) | nextVa;
			bool isTable = level < 3 && (oldPte & 0x3) == 0x3;

			if (isTable) {
				// If previous entry was table unlink it, and we'll deal with flushing TLB later

				ASSERT((newPte & 0x1) == 0);
				atomic_set64((int64*) pte, newPte);
			} else if ((oldPte & 0x1) == 0) {
				// If previous entry was invalid we can write new PTE directly

				atomic_set64((int64*) pte, newPte);
				asm("dsb ishst"); // Ensure PTE write completed
			} else if ((newPte & 0x1) == 0 || (oldPte & kPteTLBCompatMask) == (newPte & kPteTLBCompatMask)) {
				// Previous entry was valid but can be legally changed without break-before-make

				// Atomic swap in case access or modified bits were modified either by HW or software fault handler
				if ((uint64_t) atomic_test_and_set64((int64*) pte, newPte, oldPte) != oldPte)
					goto retry;

				asm("dsb ishst"); // Ensure PTE write completed
				asm("tlbi vaae1is, %0" :: "r" ((fullVa >> 12) & kTLBIMask));
				// TODO: Flush only currently used ASID (using vae1is)
				asm("dsb ish"); // Wait for TLB flush to complete
			} else {
				// Previous entry was valid and break-before-make TLB maintenance rules must be followed

				// Atomic swap in case access or modified bits were modified either by HW or software fault handler
				if ((uint64_t) atomic_test_and_set64((int64*) pte, 0, oldPte) != oldPte)
					goto retry;

				asm("dsb ishst"); // Ensure PTE write completed
				asm("tlbi vaae1is, %0" :: "r" ((fullVa >> 12) & kTLBIMask));
				asm("dsb ish"); // Wait for TLB flush to complete

				// TODO: Other core could generate spurious fault now,
				// is this something we care about?

				atomic_set64((int64*) pte, newPte);
				asm("dsb ishst"); // Ensure PTE write completed
			}

			if (isTable) {
				FreeTable(oldPte & kPteAddrMask, nextVa, level + 1, entryRemoved);
			} else if ((oldPte & 0x1) != 0) {
				entryRemoved(level, oldPte);
			}

			asm("isb"); // Flush any instructions decoded using old mapping
		} else {
			// Otherwise handle mapping in next-level table
			phys_addr_t table = MakeTable(ptPa, level, index, reservation);
			if (table != 0)
				ProcessRange(table, level + 1, nextVa, targetPa, entrySize, reservation, action, makePte, entryRemoved);
		}
		nextVa += entrySize;
	}

	// Handle misaligned tail area (or entirety of small area) in next-level table
	if (nextVa < end) {
		index = (nextVa >> shift) & tableMask;
		phys_addr_t table = MakeTable(ptPa, level, index, reservation);
		if (table != 0)
			ProcessRange(table, level + 1, nextVa, pa + (nextVa - va), end - nextVa, reservation, action, makePte, entryRemoved);
	}
}


uint8_t
VMSAv8TranslationMap::MairIndex(uint8_t type)
{
	for (int i = 0; i < 8; i++)
		if (((fMair >> (i * 8)) & 0xff) == type)
			return i;

	panic("MAIR entry not found");
	return 0;
}


uint64_t
VMSAv8TranslationMap::GetMemoryAttr(uint32 attributes, uint32 memoryType, bool isKernel)
{
	uint64_t attr = 0;

	if (!isKernel)
		attr |= kAttrNG;

	if ((attributes & B_EXECUTE_AREA) == 0)
		attr |= kAttrUXN;
	if ((attributes & B_KERNEL_EXECUTE_AREA) == 0)
		attr |= kAttrPXN;

	// AP2 forbids writes
	// AP1 allows user access

	// SWDBM is software reserved bit that we use to mark that
	// writes are allowed, and fault handler should clear AP2.
	// In that case AP2 doubles as not-dirty bit.
	// Additionally dirty state can be stored in SWDIRTY, in order not to lose
	// dirty state when changing protection from RW to RO.

	// User-Execute implies User-Read, because it would break PAN otherwise

	if ((attributes & B_READ_AREA) == 0 && (attributes & B_EXECUTE_AREA) == 0) {
		attr |= kAttrAP2; // Forbid writes
		if ((attributes & B_KERNEL_WRITE_AREA) != 0)
			attr |= kAttrSWDBM; // Mark as writeable
	} else {
		attr |= kAttrAP1; // Allow user acess
		attr |= kAttrAP2; // Forbid writes
		if ((attributes & B_WRITE_AREA) != 0)
			attr |= kAttrSWDBM; // Mark as writeable
	}

	// When supported by hardware copy our SWDBM bit into DBM,
	// so that AP2 is cleared on write attempt automatically
	// without going through fault handler.
	if ((fHwFeature & HW_DIRTY) != 0 && (attr & kAttrSWDBM) != 0)
		attr |= kAttrDBM;

	attr |= kAttrSH1 | kAttrSH0; // Inner Shareable

	uint8_t type = MAIR_NORMAL_WB;

	if (memoryType & B_MTR_UC)
		type = MAIR_DEVICE_nGnRnE; // TODO: This probably should be nGnRE for PCI
	else if (memoryType & B_MTR_WC)
		type = MAIR_NORMAL_NC;
	else if (memoryType & B_MTR_WT)
		type = MAIR_NORMAL_WT;
	else if (memoryType & B_MTR_WP)
		type = MAIR_NORMAL_WT;
	else if (memoryType & B_MTR_WB)
		type = MAIR_NORMAL_WB;

	attr |= MairIndex(type) << 2;

	return attr;
}


status_t
VMSAv8TranslationMap::Map(addr_t va, phys_addr_t pa, uint32 attributes, uint32 memoryType,
	vm_page_reservation* reservation)
{
	ThreadCPUPinner pinner(thread_get_current_thread());
	// TODO: Do we need to take fLock here?

	uint64_t pageMask = (1UL << fPageBits) - 1;
	uint64_t vaMask = (1UL << fVaBits) - 1;

	ASSERT((va & pageMask) == 0);
	ASSERT((pa & pageMask) == 0);
	ASSERT(ValidateVa(va));

	uint64_t attr = GetMemoryAttr(attributes, memoryType, fIsKernel);

	// During first mapping we need to allocate root table
	if (fPageTable == 0) {
		vm_page* page = vm_page_allocate_page(reservation, PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR);
		DEBUG_PAGE_ACCESS_END(page);
		fPageTable = page->physical_page_number << fPageBits;
	}

	ProcessRange(fPageTable, fInitialLevel, va & vaMask, pa, B_PAGE_SIZE, reservation, VMAction::MAP,
		[this, attr](int level, uint64_t va, phys_addr_t pa, uint64_t oldPte) {
			return MakeBlock(pa, level, attr);
		},
		[](int level, uint64_t oldPte) {
		});

	return B_OK;
}


status_t
VMSAv8TranslationMap::Unmap(addr_t start, addr_t end)
{
	ThreadCPUPinner pinner(thread_get_current_thread());
	// TODO: Do we need to take fLock here?

	size_t size = end - start + 1;

	uint64_t pageMask = (1UL << fPageBits) - 1;
	uint64_t vaMask = (1UL << fVaBits) - 1;

	ASSERT((start & pageMask) == 0);
	ASSERT((size & pageMask) == 0);
	ASSERT(ValidateVa(start));

	ProcessRange(fPageTable, fInitialLevel, start & vaMask, 0, size, NULL, VMAction::UNMAP,
		[](int level, uint64_t va, phys_addr_t pa, uint64_t oldPte) {
			return 0;
		},
		[](int level, uint64_t oldPte) {
		});

	return B_OK;
}


status_t
VMSAv8TranslationMap::UnmapPage(VMArea* area, addr_t address, bool updatePageQueue)
{
	UnmapPages(area, address, B_PAGE_SIZE, updatePageQueue);
	return B_OK;
}


void
VMSAv8TranslationMap::UnmapPages(VMArea* area, addr_t address, size_t size, bool updatePageQueue)
{
	ThreadCPUPinner pinner(thread_get_current_thread());
	RecursiveLocker locker(fLock);

	VMAreaMappings queue;

	uint64_t pageMask = (1UL << fPageBits) - 1;
	uint64_t vaMask = (1UL << fVaBits) - 1;

	ASSERT((address & pageMask) == 0);
	ASSERT((size & pageMask) == 0);
	ASSERT(ValidateVa(address));

	ProcessRange(fPageTable, fInitialLevel, address & vaMask, 0, size, NULL, VMAction::UNMAP,
		[](int level, uint64_t va, phys_addr_t pa, uint64_t oldPte) {
			return 0;
		},
		[this, &queue, area, updatePageQueue](int level, uint64_t oldPte) {
			ASSERT(level == 3);
			if (area->cache_type == CACHE_TYPE_DEVICE)
				return;

			vm_page* page = vm_lookup_page((oldPte & kPteAddrMask) >> fPageBits);
			DEBUG_PAGE_ACCESS_START(page);

			page->accessed |= (oldPte & kAttrAF) != 0;
			page->modified |= (oldPte & kAttrAP2) == 0 || (oldPte & kAttrSWDIRTY) != 0;

			vm_page_mapping* mapping = NULL;
			if (area->wiring == B_NO_LOCK) {
				vm_page_mappings::Iterator iterator = page->mappings.GetIterator();
				while ((mapping = iterator.Next()) != NULL) {
					if (mapping->area == area) {
						area->mappings.Remove(mapping);
						page->mappings.Remove(mapping);
						queue.Add(mapping);
						break;
					}
				}
			} else
				page->DecrementWiredCount();

			if (!page->IsMapped()) {
				atomic_add(&gMappedPagesCount, -1);

				if (updatePageQueue) {
					if (page->Cache()->temporary)
						vm_page_set_state(page, PAGE_STATE_INACTIVE);
					else if (page->modified)
						vm_page_set_state(page, PAGE_STATE_MODIFIED);
					else
						vm_page_set_state(page, PAGE_STATE_CACHED);
				}
			}

			DEBUG_PAGE_ACCESS_END(page);
		});

	locker.Unlock();
	pinner.Unlock();

	uint32 freeFlags = CACHE_DONT_WAIT_FOR_MEMORY
		| (fIsKernel ? CACHE_DONT_LOCK_KERNEL_SPACE : 0);
	while (vm_page_mapping* mapping = queue.RemoveHead())
		object_cache_free(gPageMappingsObjectCache, mapping, freeFlags);
}


void
VMSAv8TranslationMap::UnmapArea(VMArea* area, bool deletingAddressSpace, bool ignoreTopCachePageFlags)
{
	UnmapPages(area, area->Base(), area->Size(), true);
}


bool
VMSAv8TranslationMap::WalkTable(
	phys_addr_t ptPa, int level, addr_t va, phys_addr_t* pa, uint64_t* rpte)
{
	int tableBits = fPageBits - 3;
	uint64_t tableMask = (1UL << tableBits) - 1;

	int shift = tableBits * (3 - level) + fPageBits;
	uint64_t entrySize = 1UL << shift;
	uint64_t entryMask = entrySize - 1;

	int index = (va >> shift) & tableMask;

	uint64_t pte = TableFromPa(ptPa)[index];
	int type = pte & 0x3;

	if ((type & 0x1) == 0)
		return false; // Invalid entry

	uint64_t addr = pte & kPteAddrMask;
	if (level < 3) {
		if (type == 0x3) {
			// Nested table
			return WalkTable(addr, level + 1, va, pa, rpte);
		} else {
			// Hugepage entry
			*pa = addr | (va & entryMask);
			*rpte = pte;
			return true;
		}
	} else {
		// Entry at the final level
		ASSERT(type == 0x3);
		*pa = addr;
		*rpte = pte;
		return true;
	}
}


bool
VMSAv8TranslationMap::ValidateVa(addr_t va)
{
	uint64_t vaMask = (1UL << fVaBits) - 1;
	bool kernelAddr = (va & (1UL << 63)) != 0;
	if (kernelAddr != fIsKernel)
		return false;
	if ((va & ~vaMask) != (fIsKernel ? ~vaMask : 0))
		return false;
	return true;
}


status_t
VMSAv8TranslationMap::Query(addr_t va, phys_addr_t* pa, uint32* flags)
{
	ThreadCPUPinner pinner(thread_get_current_thread());

	ASSERT(ValidateVa(va));

	uint64_t pte = 0;
	bool ret = WalkTable(fPageTable, fInitialLevel, va, pa, &pte);

	uint32 result = 0;

	if (ret) {
		result |= PAGE_PRESENT;

		if ((pte & kAttrAF) != 0)
			result |= PAGE_ACCESSED;
		if ((pte & kAttrAP2) == 0 || (pte & kAttrSWDIRTY) != 0)
			result |= PAGE_MODIFIED;

		if ((pte & kAttrUXN) == 0)
			result |= B_EXECUTE_AREA;
		if ((pte & kAttrPXN) == 0)
			result |= B_KERNEL_EXECUTE_AREA;

		result |= B_KERNEL_READ_AREA;

		if ((pte & kAttrAP1) != 0)
			result |= B_READ_AREA;

		if ((pte & kAttrSWDBM) != 0) {
			result |= B_KERNEL_WRITE_AREA;

			if ((pte & kAttrAP1) != 0)
				result |= B_WRITE_AREA;
		}
	}

	*flags = result;
	return B_OK;
}


status_t
VMSAv8TranslationMap::QueryInterrupt(
	addr_t virtualAddress, phys_addr_t* _physicalAddress, uint32* _flags)
{
	return Query(virtualAddress, _physicalAddress, _flags);
}


status_t
VMSAv8TranslationMap::Protect(addr_t start, addr_t end, uint32 attributes, uint32 memoryType)
{
	ThreadCPUPinner pinner(thread_get_current_thread());

	size_t size = end - start + 1;

	uint64_t pageMask = (1UL << fPageBits) - 1;
	uint64_t vaMask = (1UL << fVaBits) - 1;

	ASSERT((start & pageMask) == 0);
	ASSERT((size & pageMask) == 0);
	ASSERT(ValidateVa(start));

	uint64_t attr = GetMemoryAttr(attributes, memoryType, fIsKernel);
	ProcessRange(fPageTable, fInitialLevel, start & vaMask, 0, size, NULL, VMAction::MODIFY,
		[this, attr](int level, uint64_t va, phys_addr_t pa, uint64_t oldPte) {
			uint64_t newAttr = attr;

			if ((oldPte & kAttrAF) != 0)
				newAttr |= kAttrAF;

			if ((oldPte & kAttrAP2) == 0 || (oldPte & kAttrSWDIRTY) != 0) {
				newAttr |= kAttrSWDIRTY;
				if ((newAttr & kAttrSWDBM) != 0)
					newAttr &= ~kAttrAP2;
			}

			return MakeBlock(oldPte & kPteAddrMask, level, newAttr);
		},
		[](int level, uint64_t oldPte) {
		});

	return B_OK;
}


status_t
VMSAv8TranslationMap::ClearFlags(addr_t va, uint32 flags)
{
	ThreadCPUPinner pinner(thread_get_current_thread());

	uint64_t pageMask = (1UL << fPageBits) - 1;
	uint64_t vaMask = (1UL << fVaBits) - 1;

	ASSERT((va & pageMask) == 0);
	ASSERT(ValidateVa(va));

	ProcessRange(fPageTable, fInitialLevel, va & vaMask, 0, B_PAGE_SIZE, NULL, VMAction::MODIFY,
		[this, flags](int level, uint64_t va, phys_addr_t pa, uint64_t oldPte) {
			uint64_t attr = oldPte & kPteAttrMask;

			if ((flags & PAGE_ACCESSED) != 0)
				attr &= ~kAttrAF;

			if ((flags & PAGE_MODIFIED) != 0) {
				attr &= ~kAttrSWDIRTY;
				attr |= kAttrAP2;
			}

			return MakeBlock(oldPte & kPteAddrMask, level, attr);
		},
		[](int level, uint64_t oldPte) {
		});

	return B_OK;
}


bool
VMSAv8TranslationMap::ClearAccessedAndModified(
	VMArea* area, addr_t address, bool unmapIfUnaccessed, bool& _modified)
{
	ThreadCPUPinner pinner(thread_get_current_thread());
	RecursiveLocker locker(fLock);

	VMAreaMappings queue;

	uint64_t pageMask = (1UL << fPageBits) - 1;
	uint64_t vaMask = (1UL << fVaBits) - 1;
	ASSERT((address & pageMask) == 0);
	ASSERT(ValidateVa(address));

	ProcessRange(fPageTable, fInitialLevel, address & vaMask, 0, B_PAGE_SIZE, NULL, VMAction::MODIFY,
		[this, unmapIfUnaccessed, &_modified](int level, uint64_t va, phys_addr_t pa, uint64_t oldPte) {
			_modified = (oldPte & kAttrAP2) == 0 || (oldPte & kAttrSWDIRTY) != 0;

			if (unmapIfUnaccessed && (oldPte & kAttrAF) == 0)
				return 0UL;

			uint64_t attr = oldPte & kPteAttrMask;

			attr &= ~kAttrAF;
			attr &= ~kAttrSWDIRTY;
			attr |= kAttrAP2;

			return MakeBlock(oldPte & kPteAddrMask, level, attr);
		},
		[this, unmapIfUnaccessed, &queue, area](int level, uint64_t oldPte) {
			ASSERT(level == 3);

			if (!unmapIfUnaccessed || (oldPte & kAttrAF) != 0)
				return;

			if (area->cache_type == CACHE_TYPE_DEVICE)
				return;

			vm_page* page = vm_lookup_page((oldPte & kPteAddrMask) >> fPageBits);
			DEBUG_PAGE_ACCESS_START(page);

			vm_page_mapping* mapping = NULL;
			if (area->wiring == B_NO_LOCK) {
				vm_page_mappings::Iterator iterator = page->mappings.GetIterator();
				while ((mapping = iterator.Next()) != NULL) {
					if (mapping->area == area) {
						area->mappings.Remove(mapping);
						page->mappings.Remove(mapping);
						queue.Add(mapping);
						break;
					}
				}
			} else
				page->DecrementWiredCount();

			if (!page->IsMapped())
				atomic_add(&gMappedPagesCount, -1);

			DEBUG_PAGE_ACCESS_END(page);
		});

	locker.Unlock();
	pinner.Unlock();

	uint32 freeFlags = CACHE_DONT_WAIT_FOR_MEMORY | CACHE_DONT_LOCK_KERNEL_SPACE;
	while (vm_page_mapping* mapping = queue.RemoveHead())
		object_cache_free(gPageMappingsObjectCache, mapping, freeFlags);

	return B_OK;
}


void
VMSAv8TranslationMap::Flush()
{
	// Necessary invalidation is performed during mapping,
	// no need to do anything more here.
}
