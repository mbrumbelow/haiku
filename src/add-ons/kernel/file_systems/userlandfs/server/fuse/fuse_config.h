/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU LGPLv2.
  See the file COPYING.LIB
*/
#ifndef USERLAND_FS_FUSE_CONFIG_H
#define USERLAND_FS_FUSE_CONFIG_H

#include "fuse_api.h"


#ifdef __cplusplus
extern "C" {
#endif

int fuse_parse_lib_config_args(struct fuse_args* args,
	struct fuse_config* config);

int fuse_parse_mount_config_args(struct fuse_args* args);

#ifdef __cplusplus
}
#endif


#endif	// USERLAND_FS_FUSE_CONFIG_H
