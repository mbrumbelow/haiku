/*
 * Copyright 2005-2008, Axel Dörfler, axeld@pinc-software.de
 * Copyright 2022, Jacob Secunda
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_MODULE_H
#define _KERNEL_MODULE_H


#include <kernel.h>
#include <fs/KPath.h>
#include <drivers/module.h>


struct kernel_args;


#ifdef __cplusplus
// C++ only part

class KPath;
class NotificationListener;

extern status_t start_watching_modules(const char *prefix,
	NotificationListener &listener);
extern status_t stop_watching_modules(const char *prefix,
	NotificationListener &listener);

extern status_t get_filesystem_path_for_module(const char* name, KPath* path);

extern "C" {
#endif

extern status_t unload_module(const char *path);
extern status_t load_module(const char *path, module_info ***_modules);

extern status_t module_init(struct kernel_args *args);
extern status_t module_init_post_threads(void);
extern status_t module_init_post_boot_device(bool bootingFromBootLoaderVolume);

#ifdef __cplusplus
}
#endif

#endif	/* _KRENEL_MODULE_H */
