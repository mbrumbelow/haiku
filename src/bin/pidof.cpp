/*
 * Copyright 2022, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oscar Lesta <oscar.lesta@gmail.com>
 */

#include <cstdio>
#include <cstring>

#include <OS.h>


void
usage(FILE* f=stdout)
{
	fprintf(f, "Return the process ids (PID) for the given process name.\n\n"
		"Usage:\tpidof [-hsv] process_name\n"
		"\t\t-h : Show this help.\n"
		"\t\t-s : Print only the PID for the first matching process.\n"
		"\t\t-v : Verbose output. Print full process name(s) and errors.\n"
	);
	return;
}


int
main(int argc, char** argv)
{
	bool singleShot = false;
	bool verbose = false;

	int c;
	while ((c = getopt(argc, argv, "hvs")) != -1) {
		switch (c) {
			case 'h':
				usage();
				return 0;
			case 's':
				singleShot = true;
				break;
			case 'v':
				verbose = true;
				break;
			default:
				usage(stderr);
				return 1;
		}
	}

	if ((argc - optind) == 0 || (argc - optind) > 1) {
		if (verbose)
			fprintf(stderr, "Error: Wrong number of arguments.\n");

		return 1;
	}

	int32 teamId = 0;
	team_info teamInfo;
	bool kernelName = false;
	bool proccessFound = false;
	char* processName = argv[argc - 1];

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

		// Did we found what we were looking for?
		if (kernelName || strcmp(p, processName) == 0) {
			proccessFound = true;
			if (verbose)
				printf("%s ", teamInfo.args);

			printf("%" B_PRId32 "\n", teamInfo.team);
			if (singleShot)
				return 0;
		}
	}

	if (verbose && !proccessFound)
		fprintf(stderr, "Error: Process name not found.\n");

	return proccessFound ? 0 : 1;
}
