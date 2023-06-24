/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS__MUTEX_H_
#define _FBSD_COMPAT_SYS__MUTEX_H_


#include <lock.h>
#include <KernelExport.h>


typedef struct mutex haiku_mutex;
typedef spinlock haiku_spinlock;

struct mtx {
	int						type;
	union {
		struct {
			haiku_mutex		lock;
			thread_id		owner;
		}					mutex;
		recursive_lock		recursive;
		struct {
			haiku_spinlock	lock;
			cpu_status		state;
		}					spinlock;
	} u;
};


#endif /* _FBSD_COMPAT_SYS__MUTEX_H_ */
