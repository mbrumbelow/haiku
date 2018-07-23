/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#ifndef LOCAL_ICON_STORE_H
#define LOCAL_ICON_STORE_H

#include <String.h>
#include <File.h>
#include <Path.h>

#include "SharedBitmap.h"
#include "PackageInfo.h"


class LocalIconStore {
public:
								LocalIconStore(const BPath& path);
	virtual						~LocalIconStore();
			status_t			TryFindIconPath(const BString& pkgName,
									BPath& path, SharedBitmap::Size size) const;

private:
			bool				_HasIconStoragePath() const;
			status_t			_IdentifyIconFileAtDirectory(
									const BPath& directory,
									BPath& bestIconPath,
									SharedBitmap::Size size) const;

			BPath				fIconStoragePath;

};


#endif // LOCAL_ICON_STORE_H
