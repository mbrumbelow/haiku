/*
 * Copyright 2022, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_UTILS_H
#define PACKAGE_UTILS_H


#include <FindDirectory.h>
#include <Path.h>

#include "PackageInfo.h"

#include <package/PackageDefs.h>


class PackageUtils {
public:
	static	status_t		DeriveLocalFilePath(const PackageInfo* package,
								BPath& result);

	static	BPackageKit::BPackageInstallationLocation
							DeriveInstallLocation(const PackageInfo* package);

private:
	static	status_t		_DeriveDirectoryWhich(
								BPackageKit::BPackageInstallationLocation
									location,
								directory_which* which);
};


#endif // PACKAGE_UTILS_H
