/*
 * Copyright 2022 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LOADER_REVISION_H
#define _LOADER_REVISION_H


/** The length of the loader revision character array symbol living in haiku_loader */
#define LOADER_REVISION_LENGTH 128


#ifdef __cplusplus
extern "C" {
#endif


/** returns the system revision */
const char* get_haiku_revision(void);


#ifdef __cplusplus
}
#endif


#endif	/* _LIBROOT_SYSTEM_REVISION_H */
