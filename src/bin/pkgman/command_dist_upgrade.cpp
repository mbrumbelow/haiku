/*
 * Copyright 2020, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk <niels.reedijk@gmail.com>
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <CopyEngine.h>
#include <Entry.h>
#include <Path.h>
#include <package/PackageInfo.h>
#include <package/solver/Solver.h>
#include <package/solver/SolverPackage.h>

#include "Command.h"
#include "pkgman.h"
#include "PackageManager.h"


using namespace BPackageKit;
using namespace BPackageKit::BPrivate;


static const char* const kShortUsage =
	"  %command% <medium-path>\n"
	"    Upgrade the system from a medium.\n";

static const char* const kLongUsage =
	"Usage: %program% <medium-path>\n"
	"Upgrade the system from a medium.\n"
	"\n"
	"Options:\n"
	"  --debug <level>\n"
	"    Print debug output. <level> should be between 0 (no debug output,\n"
	"    the default) and 10 (most debug output).\n"
	"  -y\n"
	"    Non-interactive mode. Automatically confirm changes, but fail when\n"
	"    encountering problems.\n"
	"\n";


DEFINE_COMMAND(DistUpgradeCommand, "dist-upgrade", kShortUsage,
	kLongUsage, COMMAND_CATEGORY_OTHER)


class Medium {
public:
	Medium(const char* mediumPath) {
		BEntry mediumEntry = BEntry(mediumPath);
		if (mediumEntry.InitCheck() != B_OK || !mediumEntry.Exists() )
			DIE(B_BAD_VALUE, "cannot find medium at path \"%s\"", mediumPath);

		SystemPackagesPath.SetTo(mediumPath, "system/packages", true);
		status_t status = SystemPackagesPath.InitCheck();
		if (status != B_OK)
			DIE(status, "cannot find system packages on medium \"%s\"",
				mediumPath);

		OptionalPackagesPath.SetTo(mediumPath, "_packages_", true);
		status = OptionalPackagesPath.InitCheck();
		if (status != B_OK) 
			DIE(status, "cannot find optional packages on medium \"%s\"",
				mediumPath);

		BPath tempPath(mediumPath,
			"system/settings/package-repositories/Haiku");
		HaikuRepoEntry.SetTo(tempPath.Path());
		if (!HaikuRepoEntry.Exists() && !HaikuRepoEntry.IsFile())
			DIE(B_ENTRY_NOT_FOUND, "cannot find repository config for "
				"\"Haiku\" on medium %s", mediumPath);

		tempPath.SetTo(mediumPath,
			"system/settings/package-repositories/HaikuPorts");
		HaikuRepoEntry.SetTo(tempPath.Path());
		if (!HaikuRepoEntry.Exists() && !HaikuRepoEntry.IsFile())
			DIE(B_ENTRY_NOT_FOUND, "cannot find repository config for "
				"\"HaikuPorts\" on medium %s", mediumPath);
	}

	BPath	SystemPackagesPath;
	BPath	OptionalPackagesPath;
	BEntry	HaikuRepoEntry;
	BEntry	HaikuPortsRepoEntry;
};


int
DistUpgradeCommand::Execute(int argc, const char* const* argv) {
	bool interactive = true;

	while (true) {
		static struct option sLongOptions[] = {
			{ "debug", required_argument, 0, OPTION_DEBUG },
			{ "help", no_argument, 0, ' h'  },
			{ 0, 0, 0, 0 }
		};

		int c = getopt_long(argc, (char**)argv, "hy", sLongOptions, NULL);
		if (c == -1)
			break;

		if (fCommonOptions.HandleOption(c))
			continue;

		switch (c) {
			case 'h':
				PrintUsageAndExit(false);
				break;

			case 'y':
				interactive = false;
				break;

			default:
				PrintUsageAndExit(false);
				break;
		}
	}

	// The remaining argument is the path to the medium.
	if (argc != optind + 1)
		PrintUsageAndExit(true);

	// Get the path to the medium and verify it.
	const char* mediumPath = argv[optind];
	Medium medium(mediumPath);

	// TODO: check that it is an upgrade

	// Set up the package manager
	PackageManager packageManager =
		PackageManager(B_PACKAGE_INSTALLATION_LOCATION_SYSTEM, interactive);
	packageManager.SetDebugLevel(fCommonOptions.DebugLevel());

	// Add the directories that contain the packages
	packageManager.AddPackageDirectory(medium.SystemPackagesPath.Path());
	packageManager.AddPackageDirectory(medium.OptionalPackagesPath.Path());

	// Do the sync
	packageManager.Init(BPackageManager::B_ADD_INSTALLED_REPOSITORIES);
	packageManager.FullSync();

	// Copy the repository configs from the medium
	printf("Updating repository configuration files\n");
	BCopyEngine copyEngine;
	BEntry destination("/boot/system/settings/package-repositories/Haiku");
	status_t status =
		copyEngine.CopyEntry(medium.HaikuRepoEntry, destination);
	if (status != B_OK)
		printf("Error copying the Haiku repository configuration: %s",
			strerror(status));
	destination.SetTo("/boot/system/settings/package-repositories/HaikuPorts");
	status = copyEngine.CopyEntry(medium.HaikuPortsRepoEntry, destination);
	if (status != B_OK)
		printf("Error copying the HaikuPorts repository configuration: %s",
			strerror(status));

	return 0;
}
