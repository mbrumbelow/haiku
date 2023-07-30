/*
 * Copyright 2001-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *				Daniel Reinhold, danielre@users.sf.net
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <OS.h>


void
usage(void)
{
	printf("Usage: tty [-s]\n"
	       "Prints the file name for the terminal connected to standard input.\n"
	       "  -s   silent mode: no output -- only return an exit status\n");
	exit(2);
}


int
main(int argc, char *argv[])
{
	if (argc > 2) usage();

	bool silent = false;
	if (argc == 2)
	{
		if (strcmp(argv[1], "-s"))
			usage();
		silent = true;
	}

	if (!silent)
		printf("%s\n", ttyname(STDIN_FILENO));

	return 1 - isatty(STDIN_FILENO);
}
