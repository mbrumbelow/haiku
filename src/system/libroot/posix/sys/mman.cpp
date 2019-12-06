/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <sys/mman.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include <OS.h>

#include <runtime_loader/runtime_loader.h>
#include <errno_private.h>
#include <syscall_utils.h>
#include <syscalls.h>
#include <vm_defs.h>


static const char* kSharedMemoryDir = "/var/shared_memory/";


static bool
append_string(char*& path, size_t& bytesLeft, const char* toAppend, size_t size)
{
	if (bytesLeft <= size)
		return false;

	memcpy(path, toAppend, size);
	path += size;
	path[0] = '\0';
	bytesLeft -= size;

	return true;
}


static bool
append_string(char*& path, size_t& bytesLeft, const char* toAppend)
{
	return append_string(path, bytesLeft, toAppend, strlen(toAppend));
}


static status_t
shm_name_to_path(const char* name, char* path, size_t pathSize)
{
	if (name == NULL)
		return B_BAD_VALUE;

	// skip leading slashes
	while (*name == '/')
		name++;

	if (*name == '\0')
		return B_BAD_VALUE;

	// create the path; replace occurrences of '/' by "%s" and '%' by "%%"
	if (!append_string(path, pathSize, kSharedMemoryDir))
		return ENAMETOOLONG;

	while (const char* found = strpbrk(name, "%/")) {
		// append section that doesn't need escaping
		if (found != name) {
			if (!append_string(path, pathSize, name, found - name))
				return ENAMETOOLONG;
		}

		// append escaped char
		const char* append = (*found == '%' ? "%%" : "%s");
		if (!append_string(path, pathSize, append, 2))
			return ENAMETOOLONG;
		name = found + 1;
	}

	// append remaining string
	if (!append_string(path, pathSize, name))
		return ENAMETOOLONG;

	return B_OK;
}


// #pragma mark -


void*
mmap(void* address, size_t length, int protection, int flags, int fd,
	off_t offset)
{
	// offset and length must be page-aligned
	if (length == 0 || offset % B_PAGE_SIZE != 0) {
		__set_errno(B_BAD_VALUE);
		return MAP_FAILED;
	}

	// check anonymous mapping
	if ((flags & MAP_ANONYMOUS) != 0) {
		fd = -1;
	} else if (fd < 0) {
		__set_errno(EBADF);
		return MAP_FAILED;
	}

	// either MAP_SHARED or MAP_PRIVATE must be specified
	if (((flags & MAP_SHARED) != 0) == ((flags & MAP_PRIVATE) != 0)) {
		__set_errno(B_BAD_VALUE);
		return MAP_FAILED;
	}

	// translate mapping, address specification, and protection
	int mapping = (flags & MAP_SHARED) != 0
		? REGION_NO_PRIVATE_MAP : REGION_PRIVATE_MAP;

	uint32 addressSpec;
	if ((flags & MAP_FIXED) != 0)
		addressSpec = B_EXACT_ADDRESS;
	else if (address != NULL)
		addressSpec = B_BASE_ADDRESS;
	else
		addressSpec = B_RANDOMIZED_ANY_ADDRESS;

	uint32 areaProtection = 0;
	if ((protection & PROT_READ) != 0)
		areaProtection |= B_READ_AREA;
	if ((protection & PROT_WRITE) != 0)
		areaProtection |= B_WRITE_AREA;
	if ((protection & PROT_EXEC) != 0)
		areaProtection |= B_EXECUTE_AREA;

	// create a name for this area based on calling image
	void* addr = __builtin_return_address(0);
	char* imageName;
	char areaName[B_OS_NAME_LENGTH];
	status_t status = __gRuntimeLoader->get_nearest_symbol_at_address(
		addr, NULL, NULL, &imageName, NULL, NULL, NULL, NULL);
	if (status == B_OK)
		snprintf(areaName, sizeof(areaName), "%s mmap area", imageName);
	else
		strlcpy(areaName, "mmap area", sizeof(areaName));

	// ask the kernel to map
	area_id area = _kern_map_file(areaName, &address, addressSpec,
		length, areaProtection, mapping, true, fd, offset);
	if (area < 0) {
		__set_errno(area);
		return MAP_FAILED;
	}

	return address;
}


int
munmap(void* address, size_t length)
{
	RETURN_AND_SET_ERRNO(_kern_unmap_memory(address, length));
}


int
mprotect(void* address, size_t length, int protection)
{
	RETURN_AND_SET_ERRNO(_kern_set_memory_protection(address, length,
		protection));
}


int
msync(void* address, size_t length, int flags)
{
	RETURN_AND_SET_ERRNO_TEST_CANCEL(_kern_sync_memory(address, length, flags));
}


int
posix_madvise(void* address, size_t length, int advice)
{
	RETURN_AND_SET_ERRNO(_kern_memory_advice(address, length, advice));
}


int
mlock(void* address, size_t length) {
	area_info info;
	void* end = address + length;
	ssize_t cookie = 0;
	if (end < address) return EINVAL; // The length is negative.
	if (end == address) return 0;
	if (_kern_get_area_info(_kern_area_for(address), &info) == B_BAD_VALUE) return ENOMEM;
	/* TODO: cannot get current thread
	 * if (!thread_get_current_thread()->team->effective_uid) return EPERM;
	 */
	if (!address % B_PAGE_SIZE) return EINVAL; // The addr argument is not a multiple of {PAGESIZE}.
	/* Abolished: the address range might be smaller than the area, or it could overlap several areas
	void* current;
	size_t accumulated_size;
	while (true) {
		accumulated_size += info.size;
		if (accumulated_size > length) current = end;
		info.lock = B_FULL_LOCK;
		if (current >= end) break;
		if (_kern_get_next_area_info(0, &cookie, &info) == B_BAD_VALUE) return ENOMEM;
	}
	*/
	if (info.name == "lock area" && info.size == length) info.lock = B_FULL_LOCK;
	else _kern_create_area("lock area", &address, B_BASE_ADDRESS, length, B_FULL_LOCK, B_WRITE_AREA);
	return 0;
}


int
munlock(void* address, size_t length) {
	area_info info;
	void* end = address + length;
	ssize_t cookie = 0;
	if (end < address) return EINVAL; // The length is negative.
	if (end == address) return 0;
	if (_kern_get_area_info(_kern_area_for(address), &info) == B_BAD_VALUE) return ENOMEM;
	if (!address % B_PAGE_SIZE) return EINVAL; // The addr argument is not a multiple of {PAGESIZE}.
	/* Abolished: the address range might be smaller than the area, or it could overlap several areas
	void* current;
	size_t accumulated_size;
	while (true) {
		accumulated_size += info.size;
		if (accumulated_size > length) current = end;
		info.lock = B_NO_LOCK;
		if (current >= end) break;
		if (_kern_get_next_area_info(0, &cookie, &info) == B_BAD_VALUE) return ENOMEM;
	}
	*/
	if (info.name == "lock area" && info.size == length) info.lock = B_NO_LOCK;
	else _kern_create_area("lock area", &address, B_BASE_ADDRESS, length, B_NO_LOCK, B_WRITE_AREA);
	return 0;
}


int
shm_open(const char* name, int openMode, mode_t permissions)
{
	char path[PATH_MAX];
	status_t error = shm_name_to_path(name, path, sizeof(path));
	if (error != B_OK)
		RETURN_AND_SET_ERRNO(error);

	return open(path, openMode | FD_CLOEXEC, permissions);
}


int
shm_unlink(const char* name)
{
	char path[PATH_MAX];
	status_t error = shm_name_to_path(name, path, sizeof(path));
	if (error != B_OK)
		RETURN_AND_SET_ERRNO(error);

	return unlink(path);
}
