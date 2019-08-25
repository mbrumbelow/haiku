/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Copyright 2019, Augustin Cavalier <waddlesplash>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_SELECT_POOL_H
#define _KERNEL_SELECT_POOL_H

#include <SupportDefs.h>


typedef struct mutex mutex;
typedef struct select_pool select_pool;
typedef struct selectsync selectsync;


#ifdef __cplusplus
extern "C" {
#endif


select_pool* create_select_pool(const char* object_type);
void notify_select_pool(select_pool* pool, uint32 events);
void add_select_pool_entry(select_pool* pool, selectsync* entry);
void remove_select_pool_entry(selectsync* entry);
void destroy_select_pool(select_pool* pool);


#ifdef __cplusplus
}
#endif


#endif	// _KERNEL_SELECT_POOL_H
