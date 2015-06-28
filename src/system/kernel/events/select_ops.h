/*
 * Copyright 2015, Hamish Morrison, hamishm53@gmail.com.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef _KERNEL_SELECT_OPS_H
#define _KERNEL_SELECT_OPS_H


struct select_ops {
	status_t (*select)(int32 object, struct select_info* info, bool kernel);
	status_t (*deselect)(int32 object, struct select_info* info, bool kernel);
};

static const select_ops kSelectOps[] = {
	// B_OBJECT_TYPE_FD
	{
		select_fd,
		deselect_fd
	},

	// B_OBJECT_TYPE_SEMAPHORE
	{
		select_sem,
		deselect_sem
	},

	// B_OBJECT_TYPE_PORT
	{
		select_port,
		deselect_port
	},

	// B_OBJECT_TYPE_THREAD
	{
		select_thread,
		deselect_thread
	}
};


static const uint32 kSelectOpsCount = B_COUNT_OF(kSelectOps);

#endif
