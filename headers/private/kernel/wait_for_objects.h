/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2019, Augustin Cavalier <waddlesplash>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_WAIT_FOR_OBJECTS_H
#define _KERNEL_WAIT_FOR_OBJECTS_H

#include <Drivers.h>
#include <OS.h>

#include <select_pool.h>
#include <lock.h>


struct select_set;
struct selectsync;


typedef struct selectsync {
	struct select_set*	set;
	uint32				events;
	uint32				selected_events;

	select_pool*		pool;
	struct selectsync*	pool_next;
} selectsync;


#define EVENT_OUTPUT_ONLY_FLAGS \
	(B_EVENT_ERROR | B_EVENT_DISCONNECTED | B_EVENT_INVALID)

#define EVENT_TYPE_IS_OUTPUT_ONLY(events) \
	(((events) & EVENT_OUTPUT_ONLY_FLAGS) != 0)


#ifdef __cplusplus
extern "C" {
#endif

extern status_t	select_sync_legacy_select(void* cookie,
					device_select_hook hook, uint32* events, selectsync* sync);

extern ssize_t	_user_wait_for_objects(object_wait_info* userInfos,
					int numInfos, uint32 flags, bigtime_t timeout);


#ifdef __cplusplus
}
#endif

#endif	// _KERNEL_WAIT_FOR_OBJECTS_H
