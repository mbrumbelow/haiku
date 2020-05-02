/*
 * Copyright 2020, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander G. M. Smith <agmsmith@ncf.ca>
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <getopt.h>
#include <package/manager/Exceptions.h>
#include <ObjectList.h>
#include <package/solver/SolverPackage.h>
#include <package/solver/SolverPackageSpecifier.h>
#include <package/solver/SolverPackageSpecifierList.h>
#include <stdio.h>
#include <stdlib.h>

#include "Command.h"
#include "pkgman.h"
#include "PackageManager.h"


using namespace BPackageKit;
using namespace BPackageKit::BPrivate;
using namespace BPackageKit::BManager::BPrivate;


static const char* const kShortUsage =
	"  %command%\n"
	"    Runs first boot processing for all active packages.\n";

static const char* const kLongUsage =
	"Usage: %program% %command%\n"
	"Runs first boot processing for all active packages in the selected\n"
	"package volume.  That means creating users, config files and running\n"
	"install scripts for the packages.  Normally only done after a fresh\n"
	"operating system install has placed the package files on the hard\n"
	"drive but not done anything more.  Invoked by launch_daemon.\n"
	"\n"
	"Options:\n"
	"  --debug <level>\n"
	"    Print debug output. <level> should be between 0 (no debug output,\n"
	"    the default) and 10 (most debug output).\n"
	"  -H, --home\n"
	"    Run the processing for the packages in the user's home directory.\n"
	"    Default is to do it for active packages in the system directory.\n"
	"\n";


DEFINE_COMMAND(FirstBootProcessingCommand, "first-boot-processing", kShortUsage,
	kLongUsage, COMMAND_CATEGORY_OTHER)


int
FirstBootProcessingCommand::Execute(int argc, const char* const* argv)
{
	BPackageInstallationLocation location
		= B_PACKAGE_INSTALLATION_LOCATION_SYSTEM;

	while (true) {
		static struct option sLongOptions[] = {
			{ "debug", required_argument, 0, OPTION_DEBUG },
			{ "help", no_argument, 0, 'h' },
			{ "home", no_argument, 0, 'H' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "hH", sLongOptions, NULL);
		if (c == -1)
			break;

		if (fCommonOptions.HandleOption(c))
			continue;

		switch (c) {
			case 'h':
				PrintUsageAndExit(false);
				break;

			case 'H':
				location = B_PACKAGE_INSTALLATION_LOCATION_HOME;
				break;

			default:
				PrintUsageAndExit(true);
				break;
		}
	}

	// perform the installation
	PackageManager packageManager(location, false /* interactive */);
	packageManager.SetDebugLevel(fCommonOptions.DebugLevel());
	packageManager.FirstBootProcessing(); // Errors handled with a throw.

	return 0;
}
