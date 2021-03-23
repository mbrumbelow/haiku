/*
 * Copyright 2020-2021 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT license.
 *
 * Authors:
 *    Alexander von Gluck IV <kallisti5@unixzen.com>
 *
 */


#include <drm.h>
#include <string.h>

#include "drm_common.h"


status_t
drm_get_version(struct drm_version* version)
{
	version->version_major = 0;
	version->version_minor = 0;
	version->version_patchlevel = 0;
	return B_OK;
}


status_t
drm_control_ioctl(uint32 op, void* buffer, size_t bufferLength)
{
	switch (op) {
		case DRM_IOCTL_VERSION:
		{
			if (sizeof(struct drm_version) != bufferLength)
				return B_DEV_INVALID_IOCTL;

			struct drm_version version;
			drm_get_version(&version);
			memcpy(buffer, &version, bufferLength);
			break;
		}
		default:
			return B_DEV_INVALID_IOCTL;
	};
	return B_OK;
}
