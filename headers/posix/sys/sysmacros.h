/*
 * Copyright 2018 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYS_SYSMACROS_H_
#define _SYS_SYSMACROS_H_


#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern dev_t haiku_dev_makedev(const unsigned int ma, const unsigned int mi);
extern unsigned int haiku_dev_major(const dev_t devnum);
extern unsigned int haiku_dev_minor(const dev_t devnum);

#define makedev(ma, mi) haiku_dev_makedev(ma, mi)
#define major(devnum) haiku_dev_major(devnum)
#define minor(devnum) haiku_dev_minor(devnum)

#ifdef __cplusplus
}
#endif

#endif	/* _SYS_SYSMACROS_H_ */
