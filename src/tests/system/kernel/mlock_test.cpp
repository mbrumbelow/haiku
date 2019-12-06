#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>

int
main() {
	printf("MLOCK UNIT TEST\n");
	int c = 1, res = 0;
	printf("C1: %d\n", c);
	if (c != 1) exit(1);
	res = mlock(&c, sizeof(c));
	printf("MLOCK RETURN: %d\n", res);
	if (res) exit(1);
	c = 2;
	printf("C2: %d\n", c);
	if (c != 2) exit(1);
	res = munlock(&c, sizeof(c));
	printf("MUNLOCK RETURN: %d\n", res);
	if (res) exit(1);
	c = 3;
	printf("C3: %d\n", c);
	if (c != 3) exit(1);
	printf("SUCCESS\n");
}