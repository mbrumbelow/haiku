/*
 * Copyright 2018, Haiku Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_COMPAT_USER_TIMER_DEFS_H
#define _SYSTEM_COMPAT_USER_TIMER_DEFS_H


struct compat_user_timer_info {
	bigtime_t	remaining_time;
	bigtime_t	interval;
	uint32		overrun_count;
} _PACKED;


inline status_t
copy_ref_var_to_user(user_timer_info &info, user_timer_info* userInfo)
{
	if (!IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		compat_user_timer_info compat_info;
		compat_info.remaining_time = info.remaining_time;
		compat_info.interval = info.interval;
		compat_info.overrun_count = info.overrun_count;
		if (user_memcpy(userInfo, &compat_info, sizeof(compat_info))
				< B_OK) {
			return B_BAD_ADDRESS;
		}
	} else if (user_memcpy(userInfo, &info, sizeof(info)) < B_OK) {
		return B_BAD_ADDRESS;
	}
	return B_OK;
}


#endif // _SYSTEM_COMPAT_USER_TIMER_DEFS_H