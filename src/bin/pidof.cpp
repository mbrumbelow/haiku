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
#include <util/KMessage.h>

#include <extended_system_info.h>
#include <extended_system_info_defs.h>


void
usage(FILE* f=stdout)
{
	fprintf(f, "Return the process ids (PID) for the given process name.\n\n"
		"Usage:\tpidof [-hsv] process_name\n"
		"\t\t-h : Show this help.\n"
		"\t\t-s : Print only the PID for the first matching process.\n"
		"\t\t-v : Verbose output.\n"
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

	team_id teamId = 0;
	team_info teamInfo;
	KMessage extendedInfo;
	bool proccessFound = false;
	const char* processName = argv[argc - 1];

	while (get_next_team_info(&teamId, &teamInfo) == B_OK) {
		get_extended_team_info(teamId - 1, B_TEAM_INFO_BASIC, extendedInfo);

		const char* teamName = NULL;
		if (extendedInfo.FindString("name", &teamName) != B_OK)
			continue;

		if (strcmp(teamName, processName) == 0) {
			proccessFound = true;
			printf("%" B_PRId32 "\n", teamId);
			if (singleShot)
				return 0;
		}
	}

	if (verbose && !proccessFound)
		fprintf(stderr, "Error: Process name not found.\n");

	return proccessFound ? 0 : 1;
}
