/*
 * Copyright 2022, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oscar Lesta <oscar.lesta@gmail.com>
 */

#include <stdio.h>
#include <string.h>

#include <OS.h>


void usage(void);


void
usage(void)
{
	printf("Return the process id (PID) for the given process name.\n\n"
		"Usage:\tpidof [-hv] process_name\n"
		"\t\t-h : Show this help.\n"
		"\t\t-v : Verbose output. Prints full process name and/or errors.\n"
	);
	return;
}


int
main(int argc, char** argv)
{
	team_info t_info;
	int32 t_id = 0;
	bool verbose = false;
	bool kernel_name = false;
	char* proc_name = NULL;

	int c;

	while ((c = getopt(argc, argv, "-hv")) != EOF) {
		switch (c) {
			case 'h':
				usage();
				return B_OK;
			case 'v':
				verbose = true;
				break;
		}
	}

	if (argc < 2  || argc > 3 || (argc == 2 && verbose)) {
		if (verbose) {
			printf("Error: Wrong number of arguments.\n");
		}
		return B_ERROR;
	}

	proc_name = argv[argc - 1];

	while (get_next_team_info(&t_id, &t_info) >= B_OK) {
		char* p = t_info.args;

		// Strip everything after the fist ' ' (leaving only argv[0]):
		if ((p = strchr(p, ' ')))
			*p = '\0';

		// Skip everything before the path separator:
		p = strrchr(t_info.args, '/');
		if (p == NULL) {
			// p is NULL for "kernel_team"
			if (strcmp(proc_name, "kernel_team") != 0) {
				continue;
			}
			kernel_name = true;
		} else {
			p += 1;	//  Skip path separator too.
		}

		// Did we found what we're looking for:
		if (kernel_name || strcmp(p, proc_name) == 0) {
			if (verbose) {
				printf("%s ", t_info.args);
			}
			printf("%" B_PRId32 "\n", t_info.team);
			return B_OK;
		}
	}

	if (verbose) {
		printf("Error: Process name not found.\n");
	}

	return B_NAME_NOT_FOUND; // Use ENOENT instead?
}
