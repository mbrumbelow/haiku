/*
 * Copyright 2018, Haiku Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_COMPAT_SIGNAL_H
#define _KERNEL_COMPAT_SIGNAL_H


#include <thread.h>


union compat_sigval {
	int		sival_int;
	uint32	sival_ptr;
};


inline status_t
copy_ref_var_from_user(union sigval* userSigval, union sigval &sigval)
{
	if (!IS_USER_ADDRESS(userSigval))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		compat_sigval compat_sigval;
		if (user_memcpy(&compat_sigval, userSigval, sizeof(compat_sigval))
				< B_OK) {
			return B_BAD_ADDRESS;
		}
		sigval.sival_ptr = (void*)(addr_t)compat_sigval.sival_ptr;
	} else if (user_memcpy(&sigval, userSigval, sizeof(union sigval)) < B_OK) {
		return B_BAD_ADDRESS;
	}
	return B_OK;
}


struct compat_sigaction {
	union {
		uint32		sa_handler;
		uint32		sa_sigaction;
	};
	sigset_t				sa_mask;
	int						sa_flags;
	uint32					sa_userdata;	/* will be passed to the signal
											   handler, BeOS extension */
} _PACKED;


inline status_t
copy_ref_var_from_user(struct sigaction* userAction, struct sigaction &action)
{
	if (!IS_USER_ADDRESS(userAction))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		struct compat_sigaction compat_action;
		if (user_memcpy(&compat_action, userAction, sizeof(compat_sigaction))
				< B_OK) {
			return B_BAD_ADDRESS;
		}
		action.sa_handler = (__sighandler_t)(addr_t)compat_action.sa_handler;
		action.sa_mask = compat_action.sa_mask;
		action.sa_flags = compat_action.sa_flags;
		action.sa_userdata = (void*)(addr_t)compat_action.sa_userdata;
	} else if (user_memcpy(&action, userAction, sizeof(action)) < B_OK) {
		return B_BAD_ADDRESS;
	}
	return B_OK;
}


inline status_t
copy_ref_var_to_user(struct sigaction &action, struct sigaction* userAction)
{
	if (!IS_USER_ADDRESS(userAction))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		struct compat_sigaction compat_action;
		compat_action.sa_handler = (addr_t)action.sa_handler;
		compat_action.sa_mask = action.sa_mask;
		compat_action.sa_flags = action.sa_flags;
		compat_action.sa_userdata = (addr_t)action.sa_userdata;
		if (user_memcpy(userAction, &compat_action, sizeof(compat_action))
				< B_OK) {
			return B_BAD_ADDRESS;
		}
	} else if (user_memcpy(userAction, &action, sizeof(action)) < B_OK) {
		return B_BAD_ADDRESS;
	}
	return B_OK;
}


struct compat_sigevent {
	int				sigev_notify;	/* notification type */
	int				sigev_signo;	/* signal number */
	union compat_sigval	sigev_value;	/* user-defined signal value */
	uint32			sigev_notify_function;
									/* notification function in case of
									   SIGEV_THREAD */
	uint32			sigev_notify_attributes;
									/* pthread creation attributes in case of
									   SIGEV_THREAD */
} _PACKED;


static_assert(sizeof(compat_sigevent) == 20,
	"size of compat_sigevent mismatch");


inline status_t
copy_ref_var_from_user(struct sigevent* userEvent, struct sigevent &event)
{
	if (!IS_USER_ADDRESS(userEvent))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		struct compat_sigevent compat_event;
		if (user_memcpy(&compat_event, userEvent, sizeof(compat_sigevent))
				< B_OK) {
			return B_BAD_ADDRESS;
		}
		event.sigev_notify = compat_event.sigev_notify;
		event.sigev_signo = compat_event.sigev_signo;
		event.sigev_value.sival_ptr =
			(void*)(addr_t)compat_event.sigev_value.sival_ptr;
		event.sigev_notify_function =
			(void(*)(sigval))(addr_t)compat_event.sigev_notify_function;
		event.sigev_notify_attributes =
			(pthread_attr_t*)(addr_t)compat_event.sigev_notify_attributes;
	} else if (user_memcpy(&event, userEvent, sizeof(event)) < B_OK) {
		return B_BAD_ADDRESS;
	}
	return B_OK;
}


typedef struct __compat_siginfo_t {
	int				si_signo;	/* signal number */
	int				si_code;	/* signal code */
	int				si_errno;	/* if non zero, an error number associated with
								   this signal */
	pid_t			si_pid;		/* sending process ID */
	uid_t			si_uid;		/* real user ID of sending process */
	uint32			si_addr;	/* address of faulting instruction */
	int				si_status;	/* exit value or signal */
	uint32			si_band;	/* band event for SIGPOLL */
	union compat_sigval	si_value;	/* signal value */
} compat_siginfo_t;


static_assert(sizeof(compat_siginfo_t) == 36,
	"size of compat_siginfo_t mismatch");


inline status_t
copy_ref_var_to_user(siginfo_t &info, siginfo_t* userInfo)
{
	if (!IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		compat_siginfo_t compat_info;
		compat_info.si_signo = info.si_signo;
		compat_info.si_code = info.si_code;
		compat_info.si_errno = info.si_errno;
		compat_info.si_pid = info.si_pid;
		compat_info.si_uid = info.si_uid;
		compat_info.si_addr = (addr_t)info.si_addr;
		compat_info.si_status = info.si_status;
		compat_info.si_band = info.si_band;
		compat_info.si_value.sival_ptr = (addr_t)info.si_value.sival_ptr;
		if (user_memcpy(userInfo, &compat_info, sizeof(compat_info))
				< B_OK) {
			return B_BAD_ADDRESS;
		}
	} else if (user_memcpy(userInfo, &info, sizeof(info)) < B_OK) {
		return B_BAD_ADDRESS;
	}
	return B_OK;
}


#endif // _KERNEL_COMPAT_SIGNAL_H
