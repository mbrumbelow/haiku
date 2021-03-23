/*
 * Copyright 2020-2021 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */


#include <Drivers.h>
#include <drm.h>


#if defined(__cplusplus)
extern "C" {
#endif

// Common DRM Control ioctl handler
status_t drm_control_ioctl(uint32 op, void* buffer, size_t bufferLength);


// DRM calls
status_t drm_get_version(struct drm_version* version);

#if defined(__cplusplus)
} // Extern C
#endif
