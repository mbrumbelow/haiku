/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "LocalIconStore.h"

#include <stdio.h>

#include <Directory.h>
#include <FindDirectory.h>

#include "Logger.h"
#include "ServerIconExportUpdateProcess.h"
#include "StorageUtils.h"


LocalIconStore::LocalIconStore(const BPath& path)
{
	fIconStoragePath = path;
}


LocalIconStore::~LocalIconStore()
{
}


bool
LocalIconStore::_HasIconStoragePath() const
{
	return fIconStoragePath.InitCheck() == B_OK;
}


/* This method will try to find an icon for the package name supplied.  If an
 * icon was able to be found then the method will return B_OK and will update
 * the supplied path object with the path to the icon file.
 */

status_t
LocalIconStore::TryFindIconPath(const BString& pkgName, BPath& path,
	SharedBitmap::Size size) const
{
	BPath iconPath;
	BPath iconPkgPath(fIconStoragePath);
	bool exists;
	bool isDir;

	if ( (iconPkgPath.Append("hicn") == B_OK)
		&& (iconPkgPath.Append(pkgName) == B_OK)
		&& (StorageUtils::ExistsObject(iconPkgPath, &exists, &isDir, NULL)
			== B_OK)
		&& exists
		&& isDir
		&& (_IdentifyIconFileAtDirectory(iconPkgPath, iconPath, size)
			== B_OK)
	) {
		path = iconPath;
		return B_OK;
	}

	path.Unset();
	return B_FILE_NOT_FOUND;
}


status_t
LocalIconStore::_IdentifyIconFileAtDirectory(const BPath& directory,
	BPath& iconPath, SharedBitmap::Size size) const
{
	BString iconLeafname;

	if (size == SharedBitmap::SIZE_VECTOR)
		iconLeafname = "icon.hvif";
	else if (size == SharedBitmap::SIZE_64)
		iconLeafname = "64.png";
	else if (size == SharedBitmap::SIZE_32)
		iconLeafname = "32.png";
	else if (size == SharedBitmap::SIZE_16)
		iconLeafname = "16.png";

	iconPath.Unset();

	BPath workingPath(directory);
	bool exists;
	bool isDir;

	if ( (workingPath.Append(iconLeafname) == B_OK
		&& StorageUtils::ExistsObject(
			workingPath, &exists, &isDir, NULL) == B_OK)
		&& exists
		&& !isDir) {
		iconPath.SetTo(workingPath.Path());
		return B_OK;
	}

	return B_FILE_NOT_FOUND;
}
