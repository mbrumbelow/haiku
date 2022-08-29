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
	team_info teamInfo;
	int32 teamId = 0;
	bool verbose = false;
	bool kernelName = false;
	char* processName = NULL;

	int c;

	while ((c = getopt(argc, argv, "-hv")) != EOF) {
		switch (c) {
			case 'h':
				usage();
				return 0;
			case 'v':
				verbose = true;
				break;
		}
	}

	if (argc < 2  || argc > 3 || (argc == 2 && verbose)) {
		if (verbose)
			printf("Error: Wrong number of arguments.\n");

		return 1;
	}

	processName = argv[argc - 1];

	while (get_next_team_info(&teamId, &teamInfo) >= B_OK) {
		char* p = teamInfo.args;

		// Strip everything after the fist ' ' (leaving only argv[0]):
		if ((p = strchr(p, ' ')))
			*p = '\0';

		// Skip everything before the path separator:
		p = strrchr(teamInfo.args, '/');
		if (p == NULL) {
			// p is NULL for "kernel_team"
			if (strcmp(processName, "kernel_team") != 0)
				continue;

			kernelName = true;
		} else {
			p += 1;	//  Skip path separator too.
		}

		// Did we found what we're looking for:
		if (kernelName || strcmp(p, processName) == 0) {
			if (verbose)
				printf("%s ", teamInfo.args);

			printf("%" B_PRId32 "\n", teamInfo.team);
			return 0;
		}
	}

	if (verbose)
		printf("Error: Process name not found.\n");

	return 1;
}
