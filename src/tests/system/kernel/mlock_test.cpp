#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <kernel/OS.h>

#define SPACE_SIZE 2019

int
main() {
	printf("MLOCK UNIT TEST\n");
	void* space = malloc(SPACE_SIZE);
	area_id area = create_area("mlock test area", &space, B_EXACT_ADDRESS,
		SPACE_SIZE, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	area_info info;
	get_area_info(area, &info);
	if (info.ram_size != SPACE_SIZE) goto failed;
	mlock(space, SPACE_SIZE);
	get_area_info(area, &info);
	if (info.ram_size != SPACE_SIZE) goto failed;
	munlock(space, SPACE_SIZE);
	get_area_info(area, &info);
	if (info.ram_size != SPACE_SIZE) goto failed;
	printf("SUCCESS\n");
	return 0;
failed:
	printf("FAILED\n");
	return 1;
}
