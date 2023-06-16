/*
 * Copyright 2023, Trung Nguyen, trungnt282910@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include <OS.h>
#include <image.h>

#include <syscalls.h>


#define REPORT_ERROR(msg, ...) \
	fprintf(stderr, "%s:%i: " msg "\n", __FILE__, __LINE__, ##__VA_ARGS__)


int
test_remap_different_protection()
{
	char *addr = NULL;

	area_id areaId = create_area("test area", (void**)&addr, B_ANY_ADDRESS,
		B_PAGE_SIZE * 3, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);

	if (areaId < B_OK) {
		REPORT_ERROR("create_area failed: %s\n", strerror(areaId));
		return 1;
	}

	addr += B_PAGE_SIZE;
	*addr = 'a';

	image_info info;
	int32 cookie = 0;
	info.type = B_LIBRARY_IMAGE;

	while (get_next_image_info(B_CURRENT_TEAM, &cookie, &info) == B_OK) {
		if (info.type == B_APP_IMAGE)
			break;
	}

	if (info.type != B_APP_IMAGE) {
		REPORT_ERROR("no app image found\n");
		return 1;
	}

	// Clobber the range without unmapping it first.
	status_t status = _kern_clone_memory(B_CURRENT_TEAM, (void**)&addr,
		B_EXACT_ADDRESS, B_PAGE_SIZE, B_READ_AREA | B_WRITE_AREA, 0, false,
		B_CURRENT_TEAM, info.text);

	if (status == B_OK) {
		REPORT_ERROR("clone_memory succeeded unexpectedly\n");
		return 1;
	}

	if (*addr != 'a') {
		REPORT_ERROR("unexpected value: %x\n", (int)*addr);
		return 1;
	}

	status = _kern_clone_memory(B_CURRENT_TEAM, (void**)&addr, B_EXACT_ADDRESS,
		B_PAGE_SIZE, B_READ_AREA | B_WRITE_AREA, 0, true, B_CURRENT_TEAM,
		info.text);

	if (status != B_OK) {
		REPORT_ERROR("clone_memory failed: %s\n", strerror(status));
		return 1;
	}

	if ((int)*addr != 0x7f) {
		REPORT_ERROR("unexpected value: %x\n", (int)*addr);
		return 1;
	}

	// This should not crash.
	*addr = 'b';

	// Somehow the same assert fails for the equivalent clone_area.
	// if (*(char*)info.text != 'b') {
	//     REPORT_ERROR("unexpected value: %x\n", (int)*(char*)info.text);
	//     return 1;
	// }

	addr -= B_PAGE_SIZE;
	munmap(addr, B_PAGE_SIZE * 3);

	return 0;
}


int
test_remap_rw_rx()
{
	void *memoryBlock = mmap(NULL, B_PAGE_SIZE * 10, PROT_NONE,
		MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (memoryBlock == MAP_FAILED) {
		REPORT_ERROR("mmap failed: %s\n", strerror(errno));
		return 1;
	}

	void *testBlock = (char*)memoryBlock + B_PAGE_SIZE * 3;

	void *rwBlock = NULL;
	status_t status = _kern_clone_memory(B_CURRENT_TEAM, &rwBlock,
		B_ANY_ADDRESS, B_PAGE_SIZE, B_READ_AREA | B_WRITE_AREA, 0, false,
		B_CURRENT_TEAM, testBlock);
	if (status != B_OK) {
		REPORT_ERROR("clone_memory failed: %s\n", strerror(status));
		return 1;
	}

	unsigned char asmBytes[] = {
#ifdef __x86_64__
		0x48, 0xc7, 0xc0, 0x00, 0x00, 0x00, 0x00, // mov 0, rax
		0xc3, // retq
#else
#error "Unsupported architecture"
#endif
	};

	memcpy(rwBlock, asmBytes, sizeof(asmBytes));

	void *rxBlock = NULL;
	status = _kern_clone_memory(B_CURRENT_TEAM, &rxBlock, B_ANY_ADDRESS,
		B_PAGE_SIZE, B_READ_AREA | B_EXECUTE_AREA, 1, false, B_CURRENT_TEAM,
		testBlock);
	if (status != B_OK) {
		REPORT_ERROR("clone_memory failed: %s\n", strerror(status));
		return 1;
	}

	int (*func)() = (int (*)())rxBlock;

	if (func() != 0) {
		REPORT_ERROR("unexpected return value: %i\n", func());
		return 1;
	}

	munmap(memoryBlock, B_PAGE_SIZE * 10);
	munmap(rwBlock, B_PAGE_SIZE);
	munmap(rxBlock, B_PAGE_SIZE);

	return 0;
}


int
test_remap_commit()
{
	// Create an enormous block of memory, for example, 256GB
	void *memoryBlock = mmap(NULL, 256 * 1024 * 1024 * 1024ULL, PROT_NONE,
		MAP_ANONYMOUS | MAP_PRIVATE | MAP_NORESERVE, -1, 0);
	if (memoryBlock == MAP_FAILED) {
		REPORT_ERROR("mmap failed: %s\n", strerror(errno));
		return 1;
	}

	void *testBlock = (char*)memoryBlock + B_PAGE_SIZE * 3;

	status_t status = mprotect(testBlock, B_PAGE_SIZE, PROT_READ | PROT_WRITE);
	if (status != 0) {
		REPORT_ERROR("mprotect failed: %s\n", strerror(errno));
		return 1;
	}

	*(char*)testBlock = 'a';

	// Remap a large block.
	status = _kern_clone_memory(B_CURRENT_TEAM, &testBlock, B_EXACT_ADDRESS,
		128 * 1024 * 1024 * 1024ULL, B_READ_AREA | B_WRITE_AREA, 0, true,
		B_CURRENT_TEAM, testBlock);
	if (status == B_OK) {
		REPORT_ERROR("clone_memory succeeded unexpectedly\n");
		return 1;
	}

	// Old data should be preserved after failure.
	if (*(char*)testBlock != 'a') {
		REPORT_ERROR("unexpected value: %x\n", *(char*)testBlock);
		return 1;
	}

	// Remap a small block.
	status = _kern_clone_memory(B_CURRENT_TEAM, &testBlock, B_EXACT_ADDRESS,
		B_PAGE_SIZE * 2, B_READ_AREA | B_WRITE_AREA, 0, true, B_CURRENT_TEAM,
		testBlock);
	if (status != B_OK) {
		REPORT_ERROR("clone_memory failed: %s\n", strerror(status));
		return 1;
	}

	// Old data should be preserved on success.
	if (*(char*)testBlock != 'a') {
		REPORT_ERROR("unexpected value: %x\n", *(char*)testBlock);
		return 1;
	}

	// Write should not crash.
	*((char*)testBlock + B_PAGE_SIZE) = 'a';

	munmap(memoryBlock, 256 * 1024 * 1024 * 1024ULL);
	return 0;
}


int
test_remap_in_place()
{
	void *memoryBlock = mmap(NULL, B_PAGE_SIZE * 10, PROT_NONE,
		MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (memoryBlock == MAP_FAILED) {
		REPORT_ERROR("mmap failed: %s\n", strerror(errno));
		return 1;
	}

	void *testBlock = (char*)memoryBlock + B_PAGE_SIZE * 3;
	status_t status = _kern_clone_memory(B_CURRENT_TEAM, &testBlock,
		B_EXACT_ADDRESS, B_PAGE_SIZE, B_READ_AREA | B_WRITE_AREA, 0, true,
		B_CURRENT_TEAM, testBlock);
	if (status != B_OK) {
		REPORT_ERROR("clone_memory failed: %s\n", strerror(status));
		return 1;
	}

	*(char*)testBlock = 'a';

	status = _kern_clone_memory(B_CURRENT_TEAM, &testBlock, B_EXACT_ADDRESS,
		B_PAGE_SIZE, B_READ_AREA, 0, true, B_CURRENT_TEAM, testBlock);
	if (status != B_OK) {
		REPORT_ERROR("clone_memory failed: %s\n", strerror(status));
		return 1;
	}

	if (*(char*)testBlock != 'a') {
		REPORT_ERROR("unexpected value: %x\n", (int)*(char*)testBlock);
		return 1;
	}

	munmap(memoryBlock, B_PAGE_SIZE * 10);
	return 0;
}


int
test_remap_transfer_clone_memory()
{
	int childToParent[2];
	int parentToChild[2];
	if (pipe(childToParent) != 0) {
		REPORT_ERROR("pipe failed: %s\n", strerror(errno));
		return 1;
	}
	if (pipe(parentToChild) != 0) {
		REPORT_ERROR("pipe failed: %s\n", strerror(errno));
		return 1;
	}

	// fork
	pid_t pid = fork();
	if (pid < 0) {
		REPORT_ERROR("fork failed: %s\n", strerror(errno));
		return 1;
	}

	const char data1[] = "clone_memory can act as transfer_area, except that "
	"it operates on memory ranges, not areas. The memory range can span "
	"numerous areas or be a slice of one area.";

	const char data2[] = "clone_memory can also be a more powerful "
	"clone_area. Not only is it not limited to areas (explained above), it "
	"is also not dependent on the B_CLONE_AREA flag. clone_memory is governed "
	"by a check of the effective UID of the user and the two involving teams. "
	"The same check is used for the debugger API, which also allows unlimited "
	"read and write access to the target team's whole address space.";

	const int READ_END = 0;
	const int WRITE_END = 1;

	if (pid > 0) {
		// parent
		close(childToParent[WRITE_END]);
		close(parentToChild[READ_END]);

		void *firstArea = mmap(NULL, B_PAGE_SIZE * 2, PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
		if (firstArea == MAP_FAILED) {
			REPORT_ERROR("mmap failed: %s\n", strerror(errno));
			return 1;
		}

		void *secondArea = mmap((char*)firstArea + B_PAGE_SIZE * 2,
			B_PAGE_SIZE, PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
		if (secondArea == MAP_FAILED) {
			REPORT_ERROR("mmap failed: %s\n", strerror(errno));
			return 1;
		}

		void *memoryBlock = (char*)firstArea + B_PAGE_SIZE;
		strcpy((char*)memoryBlock, data1);

		// Transfer a memory range to the child team
		void *addressInChild = NULL;
		status_t status = _kern_clone_memory(pid, &addressInChild,
			B_ANY_ADDRESS, B_PAGE_SIZE * 2, B_READ_AREA, 0, false,
			B_CURRENT_TEAM, memoryBlock);
		if (status != B_OK) {
			REPORT_ERROR("%i: clone_memory failed: %s\n", strerror(status));
			return 1;
		}

		write(parentToChild[WRITE_END], &addressInChild, sizeof(addressInChild));

		munmap(firstArea, B_PAGE_SIZE * 4);

		status = read(childToParent[READ_END], &addressInChild,
			sizeof(addressInChild));
		if (status != sizeof(addressInChild)) {
			REPORT_ERROR("read failed: %s\n", strerror(errno));
			return 1;
		}

		// Pull an area from the child
		memoryBlock = NULL;
		status = _kern_clone_memory(B_CURRENT_TEAM, &memoryBlock,
			B_ANY_ADDRESS, B_PAGE_SIZE * 3, B_READ_AREA | B_WRITE_AREA, 0,
			false, pid, addressInChild);
		if (status != B_OK) {
			REPORT_ERROR("%i: clone_memory failed: %s\n", strerror(status));
			return 1;
		}

		if (strcmp((char*)memoryBlock, data2) != 0) {
			REPORT_ERROR("unexpected value: %s\n", (char*)memoryBlock);
			return 1;
		}

		// Modify the pulled area.
		*((char*)memoryBlock + B_PAGE_SIZE * 2) = 'a';

		write(parentToChild[WRITE_END], &memoryBlock, 1);

		munmap(memoryBlock, B_PAGE_SIZE * 3);

		int exitStatus;
		waitpid(pid, &exitStatus, 0);

		if (!WIFEXITED(exitStatus) && WEXITSTATUS(exitStatus) != 0)
			return 1;

		close(childToParent[READ_END]);
		close(parentToChild[WRITE_END]);
	} else {
		// child
		close(childToParent[READ_END]);
		close(parentToChild[WRITE_END]);

		void *memoryBlock;
		status_t status = read(parentToChild[READ_END], &memoryBlock,
			sizeof(memoryBlock));
		if (status != sizeof(memoryBlock)) {
			REPORT_ERROR("read failed: %s\n", strerror(errno));
			exit(1);
		}

		if (strcmp((char*)memoryBlock, data1) != 0) {
			REPORT_ERROR("unexpected value: %s\n", (char*)memoryBlock);
			exit(1);
		}

		munmap(memoryBlock, B_PAGE_SIZE * 2);

		void *firstArea = mmap(NULL, B_PAGE_SIZE * 2, PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
		if (firstArea == MAP_FAILED) {
			REPORT_ERROR("mmap failed: %s\n", strerror(errno));
			exit(1);
		}

		void *secondArea = mmap((char*)firstArea + B_PAGE_SIZE * 2,
			B_PAGE_SIZE * 2, PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
		if (secondArea == MAP_FAILED) {
			REPORT_ERROR("mmap failed: %s\n", strerror(errno));
			exit(1);
		}

		memoryBlock = (char*)firstArea + B_PAGE_SIZE;
		strcpy((char*)memoryBlock, data2);

		write(childToParent[WRITE_END], &memoryBlock, sizeof(memoryBlock));

		read(parentToChild[READ_END], firstArea, 1);

		// Check if the modifications are visible.
		char *targetAddress = (char*)memoryBlock + B_PAGE_SIZE * 2;
		if (*targetAddress != 'a') {
			REPORT_ERROR("unexpected value: %c\n", *targetAddress);
			exit(1);
		}

		munmap(firstArea, B_PAGE_SIZE * 4);
		exit(0);
	}

	return 0;
}


int
test_remap_insufficient_privileges()
{
	int parentToChild[2];
	if (pipe(parentToChild) != 0) {
		REPORT_ERROR("pipe failed: %s\n", strerror(errno));
		return 1;
	}

	// fork
	pid_t pid = fork();
	if (pid < 0) {
		REPORT_ERROR("fork failed: %s\n", strerror(errno));
		return 1;
	}

	const int READ_END = 0;
	const int WRITE_END = 1;

	if (pid > 0) {
		// parent
		close(parentToChild[READ_END]);

		void *memoryBlock = mmap(NULL, B_PAGE_SIZE * 2, PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

		write(parentToChild[WRITE_END], &memoryBlock, sizeof(memoryBlock));

		munmap(memoryBlock, B_PAGE_SIZE * 2);

		int exitStatus;
		waitpid(pid, &exitStatus, 0);

		if (!WIFEXITED(exitStatus) && WEXITSTATUS(exitStatus) != 0)
			return 1;

		close(parentToChild[WRITE_END]);
	} else {
		// child
		close(parentToChild[WRITE_END]);

		void *memoryBlock;
		status_t status = read(parentToChild[READ_END], &memoryBlock,
			sizeof(memoryBlock));
		if (status != sizeof(memoryBlock)) {
			REPORT_ERROR("read failed: %s\n", strerror(errno));
			exit(1);
		}

		// Drop privileges.
		status = seteuid(1000);
		if (status != 0) {
			REPORT_ERROR("seteuid failed: %s\n", strerror(status));
			exit(1);
		}

		status = _kern_clone_memory(B_CURRENT_TEAM, &memoryBlock,
			B_ANY_ADDRESS, B_PAGE_SIZE * 2, B_READ_AREA | B_WRITE_AREA, 0,
			false, getppid(), memoryBlock);
		if (status != B_PERMISSION_DENIED) {
			REPORT_ERROR("clone_memory didn't fail: %s\n", strerror(status));
			exit(1);
		}

		exit(0);
	}

	return 0;
}


int
test_remap_reserved_range()
{
	void *memoryBlock = mmap(NULL, B_PAGE_SIZE * 4, PROT_READ | PROT_WRITE,
		MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (memoryBlock == MAP_FAILED) {
		REPORT_ERROR("mmap failed: %s\n", strerror(errno));
		return 1;
	}

	void *reservedAddress = NULL;
	status_t status = _kern_reserve_address_range((addr_t*)&reservedAddress,
		B_ANY_ADDRESS, B_PAGE_SIZE * 3);
	if (status != B_OK) {
		REPORT_ERROR("reserve_address_range failed: %s\n", strerror(status));
		return 1;
	}

	// Should fail because the address range is reserved.
	void *remapAddress = (char*)reservedAddress + B_PAGE_SIZE * 2;
	status = _kern_clone_memory(B_CURRENT_TEAM, &remapAddress, B_EXACT_ADDRESS,
		B_PAGE_SIZE * 2, B_READ_AREA | B_WRITE_AREA, 0, true, B_CURRENT_TEAM,
		(char*)memoryBlock + B_PAGE_SIZE);
	if (status == B_OK) {
		REPORT_ERROR("clone_memory succeeded unexpectedly\n");
		return 1;
	}

	// Should not fail.
	remapAddress = reservedAddress;
	status = _kern_clone_memory(B_CURRENT_TEAM, &remapAddress, B_EXACT_ADDRESS,
		B_PAGE_SIZE * 2, B_READ_AREA | B_WRITE_AREA, 0, false, B_CURRENT_TEAM,
		(char*)memoryBlock + B_PAGE_SIZE);
	if (status != B_OK) {
		REPORT_ERROR("clone_memory failed: %s\n", strerror(status));
		return 1;
	}

	munmap(memoryBlock, B_PAGE_SIZE * 4);
	munmap(reservedAddress, B_PAGE_SIZE * 2);
	_kern_unreserve_address_range((addr_t)reservedAddress, B_PAGE_SIZE * 3);

	// In-place remapping on a reserved address range.
	reservedAddress = NULL;
	status = _kern_reserve_address_range((addr_t*)&reservedAddress,
		B_ANY_ADDRESS, B_PAGE_SIZE * 4);
	if (status != B_OK) {
		REPORT_ERROR("reserve_address_range failed: %s\n", strerror(status));
		return 1;
	}

	memoryBlock = mmap(reservedAddress, B_PAGE_SIZE * 4, PROT_READ | PROT_WRITE,
		MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
	if (memoryBlock == MAP_FAILED) {
		REPORT_ERROR("mmap failed: %s\n", strerror(errno));
		return 1;
	}

	remapAddress = (char*)memoryBlock + B_PAGE_SIZE;
	status = _kern_clone_memory(B_CURRENT_TEAM, &remapAddress, B_EXACT_ADDRESS,
		B_PAGE_SIZE * 2, B_READ_AREA | B_WRITE_AREA, 0, true, B_CURRENT_TEAM,
		remapAddress);
	if (status != B_OK) {
		REPORT_ERROR("clone_memory failed: %s\n", strerror(status));
		return 1;
	}

	munmap(memoryBlock, B_PAGE_SIZE * 4);
	_kern_unreserve_address_range((addr_t)reservedAddress, B_PAGE_SIZE * 4);

	return 0;
}


int
main()
{
	if (test_remap_different_protection() != 0)
		return 1;

	if (test_remap_rw_rx() != 0)
		return 1;

	if (test_remap_commit() != 0)
		return 1;

	if (test_remap_in_place() != 0)
		return 1;

	if (test_remap_transfer_clone_memory() != 0)
		return 1;

	if (test_remap_insufficient_privileges() != 0)
		return 1;

	if (test_remap_reserved_range() != 0)
		return 1;

	return 0;
}
