/*
 * Copyright 2011, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		David Höppner <0xffea@gmail.com>
 */


#include <posix/xsi_shared_memory.h>

#include <stdio.h>

#include <sys/ipc.h>
#include <sys/types.h>

#include <kernel/debug.h>
#include <kernel/heap.h>
#include <kernel/lock.h>
#include <kernel/team.h>
#include <kernel/thread.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>
#include <vm/vm.h>
#include <vm/VMAddressSpace.h>
#include <vm/VMArea.h>
#include <vm/VMCache.h>


#define TRACE_XSI_SHM
#ifdef TRACE_XSI_SHM
#	define TRACE(x)		dprintf x
#else
#	define TRACE(x)		/* nothing */
#endif

#define MAX_XSI_SHARED_MEMORY_COUNT	64
#define MIN_XSI_SHARED_MEMORY_SIZE	1
#define MAX_XSI_SHARED_MEMORY_SIZE	(256 * 1024 * 1024)
#define MAX_XSI_SHARED_MEMORY_ATTACH	128


static mutex sXsiSharedMemoryLock = MUTEX_INITIALIZER("Global POSIX XSI Shared Memory Lock");
static int32 sSharedMemoryCount = 0;
static int32 sGlobalSequenceNumber = 1;


class SharedMemoryEntry {
public:
	SharedMemoryEntry(key_t key, size_t size, int flags)
		:
		fKey(key),
		fSize(size)
	{
		fIdDataStructure.shm_segsz = fSize;

		fIdDataStructure.shm_cpid = getpid();
		fIdDataStructure.shm_lpid = fIdDataStructure.shm_nattch = 0;
		fIdDataStructure.shm_atime = fIdDataStructure.shm_dtime = 0;
		fIdDataStructure.shm_ctime = real_time_clock();

		fIdDataStructure.shm_perm.uid = fIdDataStructure.shm_perm.cuid = geteuid();
		fIdDataStructure.shm_perm.gid = fIdDataStructure.shm_perm.cgid = getegid();
		fIdDataStructure.shm_perm.mode = flags & 0x1ff;
	}

	status_t Init()
	{
		status_t status = VMCacheFactory::CreateAnonymousCache(fCache, 1, 0, 0, true,
					VM_PRIORITY_SYSTEM);
		if (status != B_OK)
			return ENOMEM;

		fCache->temporary = 1;
		fCache->virtual_end = PAGE_ALIGN(fSize);
		fCache->committed_size = 0;

		snprintf(fAreaName, B_OS_NAME_LENGTH, "xsi_shared_memory%ld", sGlobalSequenceNumber);

		SetId();

		atomic_add(&sGlobalSequenceNumber, 1);
		atomic_add(&sSharedMemoryCount, 1);

		return B_OK;
	}

	~SharedMemoryEntry()
	{
		status_t status;

		for (VMArea* area = fCache->areas; area != NULL; area = area->cache_next) {
			status = vm_delete_area(VMAddressSpace::CurrentID(), area->id, true);
				if (status != B_OK) {
					return;
				}
		}

		fCache->Delete();

		atomic_add(&sSharedMemoryCount, -1);
	}

	void SetId();

	bool HasPermission() const
	{
		uid_t uid = geteuid();
		if (uid == 0
			|| uid == fIdDataStructure.shm_perm.uid
			|| uid == fIdDataStructure.shm_perm.cuid)
			return true;

		return false;
	}

	bool HasReadPermission() const
	{
		if ((fIdDataStructure.shm_perm.mode & S_IROTH) != 0)
			return true;

		uid_t uid = geteuid();
		if (uid == 0 || (uid == fIdDataStructure.shm_perm.uid
			&& (fIdDataStructure.shm_perm.mode & S_IRUSR) != 0))
			return true;

		gid_t gid = getegid();
		if (uid == 0 || (gid == fIdDataStructure.shm_perm.gid
			&& (fIdDataStructure.shm_perm.mode & S_IRGRP) != 0))
			return true;

		return false;
	}

	int Id() const
	{
		return fId;
	}

	key_t Key() const
	{
		return fKey;
	}

	VMCache* Cache() const
	{
		return fCache;
	}

	char* AreaName()
	{
		return fAreaName;
	}

	size_t Size() const
	{
		return fSize;
	}

	struct shmid_ds* IdDataStructure()
	{
		return &fIdDataStructure;
	}

	SharedMemoryEntry*& Link()
	{
		return fLink;
	}

private:
	int			fId;
	key_t			fKey;
	VMCache*		fCache;
	char			fAreaName[B_OS_NAME_LENGTH];
	size_t			fSize;
	struct shmid_ds		fIdDataStructure;
	SharedMemoryEntry*	fLink;
};

struct XsiSharedMemoryEntryHashTableDefinition {
	typedef int			KeyType;
	typedef SharedMemoryEntry	ValueType;

	size_t HashKey(int id) const
	{
		return (size_t)id;
	}

	size_t Hash(SharedMemoryEntry* variable) const
	{
		return (size_t)HashKey(variable->Id());
	}

	bool Compare(int id, SharedMemoryEntry* variable) const
	{
		return id == (int)variable->Id();
	}

	SharedMemoryEntry*& GetLink(SharedMemoryEntry* variable) const
	{
		return variable->Link();
	}
};

typedef BOpenHashTable<XsiSharedMemoryEntryHashTableDefinition> SharedMemoryEntryTable;
static SharedMemoryEntryTable sSharedMemoryEntryTable;


class Ipc {
public:
	Ipc(key_t key, size_t size)
		:
		fKey(key),
		fSize(size),
		fSharedMemoryId(-1)
	{
	}

	key_t Key() const
	{
		return fKey;
	}

	size_t Size() const
	{
		return fSize;
	}

	int SharedMemoryId() const
	{
		return fSharedMemoryId;
	}

	void SetSharedMemoryId(int id)
	{
		fSharedMemoryId = id;
	}

	Ipc*& Link()
	{
		return fLink;
	}

private:
	key_t	fKey;
	size_t	fSize;
	int	fSharedMemoryId;
	Ipc*	fLink;
};

struct IpcHashTableDefinition {
	typedef key_t	KeyType;
	typedef Ipc	ValueType;

	size_t HashKey(const key_t key) const
	{
		return (size_t)key;
	}

	size_t Hash(Ipc* variable) const
	{
		return (size_t)HashKey(variable->Key());
	}

	bool Compare(const key_t key, Ipc* variable) const
	{
		return (key_t)key == (key_t)variable->Key();
	}

	Ipc*& GetLink(Ipc* variable) const
	{
		return variable->Link();
	}
};

static BOpenHashTable<IpcHashTableDefinition> sIpcTable;


void
SharedMemoryEntry::SetId()
{
        fId = real_time_clock();

        while (true) {
                if (sSharedMemoryEntryTable.Lookup(fId) == NULL)
                        break;
                fId = (fId + 1) % INT_MAX;
        }
}


//      #pragma mark - Kernel exported API


void
xsi_shm_init()
{
	status_t status = sSharedMemoryEntryTable.Init();
	if (status != B_OK)
		panic("xsi_shm_init(): Failed to initialize shared memory hash table!\n");

	status = sIpcTable.Init();
	if (status != B_OK)
		panic("xsi_shm_init(): Failed to initialize ipc key hash table!\n");
}


status_t
_user_xsi_shmat(int id, const void* address, int flags, void** _returnAddress)
{
	TRACE(("shmat: id = %d, address = %p, flags = %d\n",
		id, address, flags));

	MutexLocker _(sXsiSharedMemoryLock);

	SharedMemoryEntry* entry = sSharedMemoryEntryTable.Lookup(id);
	if (entry == NULL)
		return EINVAL;

	struct shmid_ds* idDataStructure = entry->IdDataStructure();
	if (idDataStructure->shm_nattch >= MAX_XSI_SHARED_MEMORY_ATTACH)
		return EMFILE;

	int protection = B_READ_AREA | B_WRITE_AREA | B_SHARED_AREA;
	if ((flags & SHM_RDONLY) != 0)
		protection &= ~B_WRITE_AREA;

	uint32 addressSpec = B_ANY_ADDRESS;
	if (address != NULL) {
		if ((flags & SHM_RND) != 0) {
			// TODO address = (void*)(addr - ((uintptr_t)addr % SHMLBA));
			return EINVAL;
		} else {
			address = (void*)address;
			addressSpec = B_EXACT_ADDRESS;
		}

		if (!IS_USER_ADDRESS(address))
			return B_BAD_ADDRESS;
	}

	VMAddressSpace* targetAddressSpace = VMAddressSpace::GetCurrent();
	if (targetAddressSpace == NULL)
		return EINVAL;

	size_t size = PAGE_ALIGN(entry->Size());

	virtual_address_restrictions addressRestrictions = {};
	addressRestrictions.address = NULL;
	addressRestrictions.address_specification = addressSpec;
	const uint32 allocationFlags = HEAP_DONT_WAIT_FOR_MEMORY | HEAP_DONT_LOCK_KERNEL_SPACE; 

	VMArea* newArea = targetAddressSpace->CreateArea(entry->AreaName(), B_NO_LOCK,
				protection, allocationFlags);
	if (newArea == NULL)
		return ENOMEM;

	void* virtualAddress;
	status_t status = targetAddressSpace->InsertArea(newArea, size, &addressRestrictions,
				allocationFlags, &virtualAddress);
	if (status != B_OK)
		return EINVAL;

	VMCache* cache = entry->Cache();
	cache->Lock();
	newArea->cache = cache;
	newArea->cache_offset = 0;
	newArea->cache_type = CACHE_TYPE_RAM;

	cache->InsertAreaLocked(newArea);
	VMAreaHash::Insert(newArea);

	targetAddressSpace->Get();

	cache->AcquireRefLocked();
	cache->Unlock();

	idDataStructure->shm_atime = real_time_clock();
	idDataStructure->shm_nattch++;

	*_returnAddress = virtualAddress;

	return B_OK;
}


int
_user_xsi_shmctl(int id, int command, struct shmid_ds* _buffer)
{
	status_t status;

	TRACE(("shmctl: id = %d, command = %d, buffer = %p\n", id,
		command, _buffer));

	MutexLocker _(sXsiSharedMemoryLock);

	SharedMemoryEntry* entry = sSharedMemoryEntryTable.Lookup(id);
	if (entry == NULL)
		return EINVAL;

	switch (command) {
		case IPC_STAT: {
			if (!entry->HasReadPermission())
				return EACCES;

			if (!IS_USER_ADDRESS(_buffer))
				return B_BAD_ADDRESS;

			// TODO: EOVERFLOW
			struct shmid_ds* idDataStructure = entry->IdDataStructure();
			status = user_memcpy(_buffer, (const void*)idDataStructure, sizeof(struct shmid_ds));
			if (status != B_OK)
				return B_BAD_ADDRESS;
			break;
		}

		case IPC_SET: {
			if (!entry->HasPermission())
				return EPERM;

			if (!IS_USER_ADDRESS(_buffer))
				return B_BAD_ADDRESS;

			struct shmid_ds* idDataStructure = entry->IdDataStructure();
			status = user_memcpy(idDataStructure, _buffer, sizeof(struct shmid_ds));
			if (status != B_OK)
				return B_BAD_ADDRESS;
			break;
		}

		case IPC_RMID: {
			if (!entry->HasPermission())
				return EPERM;

			if (entry->Key() != IPC_PRIVATE) {
				Ipc* ipcKey = sIpcTable.Lookup(entry->Key());
				sIpcTable.Remove(ipcKey);
				delete ipcKey;
			}

			sSharedMemoryEntryTable.Remove(entry);
			atomic_add(&sSharedMemoryCount, -1);

			delete entry;
			break;
		}

		default:
			return EINVAL;
	}

	return B_OK;
}


int
_user_xsi_shmdt(const void* address)
{
	TRACE(("xsi_shmdt: detach address = %p\n", address));

	area_id areaId = area_for((void*)address);
	VMArea* area = VMAreaHash::Lookup(areaId);

	if (area < 0 || area == NULL)
		return EINVAL;

	MutexLocker _(sXsiSharedMemoryLock);

	SharedMemoryEntry* entry = NULL;
	SharedMemoryEntryTable::Iterator iterator = sSharedMemoryEntryTable.GetIterator();
	while (iterator.HasNext()) {
		entry = iterator.Next();
		if (entry->Cache() == area->cache)
			break;
	}

	status_t status = vm_delete_area(VMAddressSpace::CurrentID(), areaId, true);
	if (status != B_OK || entry == NULL)
		return EINVAL;

	struct shmid_ds* idDataStructure = entry->IdDataStructure();
	idDataStructure->shm_dtime = real_time_clock();
	idDataStructure->shm_nattch--;

	return B_OK;
}


int
_user_xsi_shmget(key_t key, size_t size, int flags)
{
	Ipc* ipcKey = NULL;

	TRACE(("shmget: key = %d, size = %" B_PRIuSIZE ", flags = %d\n",
		(int)key, size, flags));

	MutexLocker _(sXsiSharedMemoryLock);

	if (key != IPC_PRIVATE) {
		ipcKey = sIpcTable.Lookup(key);
		if (ipcKey == NULL) {
			if ((flags & IPC_CREAT) == 0)
				return ENOENT;

			ipcKey = new(std::nothrow) Ipc(key, size);
			if (ipcKey == NULL)
				return ENOMEM;

			sIpcTable.Insert(ipcKey);
		} else {
			if ((flags & IPC_CREAT) != 0 && (flags & IPC_EXCL) != 0)
				return EEXIST;

			if (ipcKey->Size() < size)
				return EINVAL;

			return ipcKey->SharedMemoryId();
		}
	}

	if (size < MIN_XSI_SHARED_MEMORY_SIZE
		|| size > MAX_XSI_SHARED_MEMORY_SIZE)
		return EINVAL;

	if (sSharedMemoryCount > MAX_XSI_SHARED_MEMORY_COUNT)
		return ENOSPC;

	SharedMemoryEntry* entry = new(std::nothrow) SharedMemoryEntry(key, size, flags);
	if (entry == NULL)
		return ENOMEM;

	status_t status = entry->Init();
	if (status != B_OK) {
		delete entry;
		return status;
	}

	if (key != IPC_PRIVATE)
		ipcKey->SetSharedMemoryId(entry->Id());

	sSharedMemoryEntryTable.Insert(entry);

	return entry->Id();
}
