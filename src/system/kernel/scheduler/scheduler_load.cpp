/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */


#include "scheduler_cpu.h"

#include <kernel.h>
#include <scheduler_defs.h>
#include <timer.h>


using namespace Scheduler;

// load average algorithm from FreeBSD, see kern_sync.c
const static int kFShift = 11;
const static long kFScale = 1 << kFShift;
static struct loadavg sAverageRunnable = {{ 0, 0, 0}, kFScale};
const static uint64 sCExp[3] = {(uint64)(0.9200444146293232 * kFScale),
	(uint64)(0.9834714538216174 * kFScale), (uint64)(0.9944598480048967 * kFScale)};
static struct timer sLoadavgTimer = {0};


static int32
_LoadavgUpdate(struct timer* timer)
{
	uint64 threadCount = 0;
	for (int i = 0; i < gCoreCount; i++)
		threadCount += gCoreEntries[i].ThreadCount();
	for (int i = 0; i < 3; i++) {
		sAverageRunnable.ldavg[i] =
			(sCExp[i] * sAverageRunnable.ldavg[i] + threadCount * kFScale * (kFScale - sCExp[i]))
			>> kFShift;
	}
	add_timer(timer, &_LoadavgUpdate, 5000000, B_ONE_SHOT_RELATIVE_TIMER);

	return B_HANDLED_INTERRUPT;
}


status_t
scheduler_loadavg_init()
{
	add_timer(&sLoadavgTimer, &_LoadavgUpdate, 5000000, B_ONE_SHOT_RELATIVE_TIMER);
	return B_OK;
}


// #pragma mark - Syscalls


status_t
_user_get_loadavg(struct loadavg* userInfo, size_t size)
{
	if (userInfo == NULL || !IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;
	if (size != sizeof(struct loadavg))
		return B_BAD_VALUE;
	if (user_memcpy(userInfo, &sAverageRunnable, sizeof(struct loadavg)) < B_OK)
		return B_BAD_ADDRESS;

	return B_OK;
}
