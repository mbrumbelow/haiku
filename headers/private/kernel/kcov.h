/*
 * Copyright 2021, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_COVERAGE_H
#define _KERNEL_COVERAGE_H


#include <Drivers.h>
#include <OS.h>


#define KCOV_MAXENTRIES		(1 << 24)
#define KCOV_ENTRY_SIZE		sizeof(uint64)

#define KCOV_MODE_TRACE_PC	0
#define KCOV_MODE_TRACE_CMP	1

#define KCOV_CMP_CONST		(1 << 0)
#define KCOV_CMP_SIZE(x)	((x) << 1)
#define KCOV_CMP_MASK		(3 << 1)
#define KCOV_CMP_GET_SIZE(x)	(((x) >> 1) & 3)


enum {
	KIOENABLE = B_DEVICE_OP_CODES_END + 1,
	KIODISABLE,
	KIOSETBUFSIZE,
	KIOGETAREA,
};


typedef enum {
	KCOV_STATE_INVALID,
	KCOV_STATE_OPEN,
	KCOV_STATE_READY,
	KCOV_STATE_RUNNING,
	KCOV_STATE_CLOSED
} kcov_state;


namespace BKernel {
	struct Thread;
}

using BKernel::Thread;


struct thread_kcov_info {
	uint64*		buffer;
	struct Thread* thread;
	size_t		count;
	area_id		area;
	int 		mode;
	kcov_state	state;
};


void kernel_coverage_init();


#endif	/* _KERNEL_COVERAGE_H */
