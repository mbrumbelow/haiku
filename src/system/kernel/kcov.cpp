/*
 * Copyright 2021, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>

#include <kcov.h>

#include "thread.h"


#define RETURN_IP	((uint64)__builtin_return_address(0))
#define KCOV_WORDS_PER_CMP	4


static bool sInitialized = false;


static void
trace_cmp(uint64 type, uint64 arg1, uint64 arg2, uint64 ret)
{
	if (!sInitialized)
		return;

	Thread *thread = thread_get_current_thread();
	if (thread == NULL || thread_is_in_interrupt(thread))
		return;

	struct thread_kcov_info* info = thread->kcov_info;
	if (info == NULL || info->state != KCOV_STATE_RUNNING
		|| info->mode != KCOV_MODE_TRACE_CMP) {
		return;
	}

	uint64* buffer = info->buffer;
	uint64 index = buffer[0];
	uint64 start = index * KCOV_WORDS_PER_CMP + 1;
	if ((start + 4) > info->count)
		return;

	buffer[start] = type;
	buffer[start + 1] = arg1;
	buffer[start + 2] = arg2;
	buffer[start + 3] = ret;
	buffer[0] = index + 1;
}


extern "C" {

void
__sanitizer_cov_trace_pc(void)
{
	if (!sInitialized)
		return;

	Thread *thread = thread_get_current_thread();
	if (thread == NULL || thread_is_in_interrupt(thread))
		return;

	struct thread_kcov_info* info = thread->kcov_info;
	if (info == NULL || info->state != KCOV_STATE_RUNNING
		|| info->mode != KCOV_MODE_TRACE_PC) {
		return;
	}

	uint64* buffer = info->buffer;
	uint64 index = buffer[0] + 1;
	if (index + 1 > info->count)
		return;
	buffer[index] = (uint64)__builtin_return_address(0);
	buffer[0] = index;
}


void
__sanitizer_cov_trace_cmp1(uint8 arg1, uint8 arg2)
{

	trace_cmp(KCOV_CMP_SIZE(0), arg1, arg2, RETURN_IP);
}


void
__sanitizer_cov_trace_cmp2(uint16 arg1, uint16 arg2)
{

	trace_cmp(KCOV_CMP_SIZE(1), arg1, arg2, RETURN_IP);
}


void
__sanitizer_cov_trace_cmp4(uint32 arg1, uint32 arg2)
{

	trace_cmp(KCOV_CMP_SIZE(2), arg1, arg2, RETURN_IP);
}


void
__sanitizer_cov_trace_cmp8(uint64 arg1, uint64 arg2)
{

	trace_cmp(KCOV_CMP_SIZE(3), arg1, arg2, RETURN_IP);
}


void
__sanitizer_cov_trace_const_cmp1(uint8 arg1, uint8 arg2)
{

	trace_cmp(KCOV_CMP_SIZE(0) | KCOV_CMP_CONST, arg1, arg2, RETURN_IP);
}


void
__sanitizer_cov_trace_const_cmp2(uint16 arg1, uint16 arg2)
{

	trace_cmp(KCOV_CMP_SIZE(1) | KCOV_CMP_CONST, arg1, arg2, RETURN_IP);
}


void
__sanitizer_cov_trace_const_cmp4(uint32 arg1, uint32 arg2)
{

	trace_cmp(KCOV_CMP_SIZE(2) | KCOV_CMP_CONST, arg1, arg2, RETURN_IP);
}


void
__sanitizer_cov_trace_const_cmp8(uint64 arg1, uint64 arg2)
{

	trace_cmp(KCOV_CMP_SIZE(3) | KCOV_CMP_CONST, arg1, arg2, RETURN_IP);
}


void
__sanitizer_cov_trace_switch(uint64 val, uint64 *cases)
{
	uint64 type;
	switch (cases[1]) {
		case 8:
			type = KCOV_CMP_SIZE(0);
			break;
		case 16:
			type = KCOV_CMP_SIZE(1);
			break;
		case 32:
			type = KCOV_CMP_SIZE(2);
			break;
		case 64:
			type = KCOV_CMP_SIZE(3);
			break;
		default:
			return;
	}

	val |= KCOV_CMP_CONST;

	uint64 count = cases[0];
	for (uint64 i = 0; i < count; i++)
		trace_cmp(type, val, cases[i + 2], RETURN_IP);
}


void
__sanitizer_cov_trace_cmpf(float arg1, float arg2)
{
	/* unused */
}


void
__sanitizer_cov_trace_cmpd(double arg1, double arg2)
{
	/* unused */
}


}


void
kernel_coverage_init()
{
	sInitialized = true;
}

