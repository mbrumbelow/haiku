/*
 * Copyright 2018, Haiku Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_COMPAT_SYS_SOCKET_H
#define _SYSTEM_COMPAT_SYS_SOCKET_H


#define compat_ptr uint32
struct compat_msghdr {
	compat_ptr	msg_name;		/* address we're using (optional) */
	socklen_t	msg_namelen;	/* length of address */
	compat_ptr	msg_iov;		/* scatter/gather array we'll use */
	int			msg_iovlen;		/* # elements in msg_iov */
	compat_ptr	msg_control;	/* extra data */
	socklen_t	msg_controllen;	/* length of extra data */
	int			msg_flags;		/* flags */
} _PACKED;


static_assert(sizeof(compat_msghdr) == 28,
	"size of compat_msghdr mismatch");


inline status_t
copy_ref_var_from_user(struct msghdr* userInfo, struct msghdr &info)
{
	if (!IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		compat_msghdr compat_info;
		if (user_memcpy(&compat_info, userInfo, sizeof(compat_info))
				< B_OK) {
			return B_BAD_ADDRESS;
		}
		info.msg_name = (void*)(addr_t)compat_info.msg_name;
		info.msg_namelen = compat_info.msg_namelen;
		info.msg_iov = (iovec*)(addr_t)compat_info.msg_iov;
		info.msg_iovlen = compat_info.msg_iovlen;
		info.msg_control = (void*)(addr_t)compat_info.msg_control;
		info.msg_controllen = compat_info.msg_controllen;
		info.msg_flags = compat_info.msg_flags;
	} else if (user_memcpy(&info, userInfo, sizeof(info)) < B_OK) {
		return B_BAD_ADDRESS;
	}
	return B_OK;
}


#endif // _SYSTEM_COMPAT_SYS_SOCKET_H