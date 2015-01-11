/*
 * Copyright 2015 Tiancheng "Timothy" Gu, timothygu99@gmail.com
 * Copyright 2001-2002 François Revol (mmu_man)
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <errno.h>
#include <sys/resource.h>

#include <OS.h>
#include <sys/param.h> // for MAX() and MIN() macros

// Part of this file is adapted from src/bin/renice.c, by François Revol.

// Some notes on different priority notations:
//
// Notation system          | Real-time | High Prio | Default | Low Prio |
// -------------------------|----------:|----------:|--------:|---------:|
// BeOS/Haiku               |      120* |        99 |      10 |      1** |
// UNIX [gs]etpriority()*** |      N.A. |       -20 |       0 |       19 |
// UNIX internal            |      N.A. |         0 |      20 |       39 |
//
// * 	Note that BeOS/Haiku does not have an absolute highest priority value
//   	(else than the maximum of int32), and (B_REAL_TIME_PRIORITY - 1) is
//   	the highest non-real-time priority and usually used as the priority
//   	limit. On UNIX systems there is no such concept as "real-time."
// **	0 only for idle_thread
// ***	Also used for UNIX nice(1) and nice(3)

#ifdef CLIP
#undef CLIP
#endif
#define CLIP(n, max, min) MAX(MIN((n), (max)), (min))

// In these contexts, "min" does not mean "lowest value," but "lowest
// priority," which is the maximum value in UNIX priority notation.
// "Zero" means normal priority, which in UNIX function notation is 0.
// POSIX specification also refers to it as "NZERO."
#ifndef NZERO
#define NZERO 20
#endif
#define NMAX 0
#define NMIN (NZERO * 2 - 1)
#define CLIP_TO_UNIX(n) CLIP(n, NMIN, NMAX)

#define BZERO B_NORMAL_PRIORITY
#define BMAX (B_REAL_TIME_DISPLAY_PRIORITY - 1)
#define BMIN 1
#define CLIP_TO_BEOS(n) CLIP(n, BMAX, BMIN)

// To accurately convert the notation values, we need to use an exponential
// function:
//
// 	f(x) = 99e^(-0.116x)
//
// where f() represents the BeOS priority value, and x represents the Unix
// priority value.
//
// But that's too complicated. And slow. So we use a simple piecewise linear
// approach here, by a simple rescaling of the values.

// returns an equivalent UNIX priority for a given BeOS priority.
static int32
prio_be_to_unix(int32 prio)
{
	int out;
	if (prio >= BZERO)
		out = NZERO
			- ((prio - BZERO) * NZERO + (BMAX - BZERO) / 2) / (BMAX - BZERO);
			// `(BMAX - BZERO) / 2` for rounding
	else
		out = NZERO
			+ ((BZERO - prio) * (NZERO - 1)) / (BZERO - BMIN)
			+ 1;
			// `+1` for rounding
	return CLIP_TO_UNIX(out);
}


// returns an equivalent BeOS priority for a given UNIX priority.
static int32
prio_unix_to_be(int32 prio)
{
	int out;
	// Do not need to care about rounding
	if (prio >= NZERO)
		out = BZERO - ((prio - NZERO) * (BZERO - BMIN)) / (NZERO - 1);
	else
		out = BZERO + ((NZERO - prio) * (BMAX - BZERO)) / (NZERO);
	return CLIP_TO_BEOS(out);
}


int
getpriority(int which, id_t who) {
	team_info team;
	thread_info thread;
	int32 team_cookie = 0, th_cookie = 0;
	int out = -100;
	bool found = false;

	if (who < 0) {
		errno = EINVAL;
		return -1;
	}
	switch (which) {
		// In Haiku process and process group are the same -- teams.
		case PRIO_PROCESS:
		case PRIO_PGRP:
		{
			status_t status;
			if (!who)
				who = getpid();
			status = get_team_info(who, &team);
			if (status == B_BAD_TEAM_ID) {
				errno = ESRCH;
				return -1;
			}
			while (get_next_thread_info(team.team, &th_cookie, &thread)
				== B_OK) {
				if (thread.priority > out) {
					found = true;
					out = thread.priority;
				}
			}
			break;
		}
		case PRIO_USER:
		{
			uid_t uid;
			// `who` (id_t) is int32, but uid_t is uint32, so using this
			// indirection to get rid of compiler warnings
			// `who` can never be negative because of the `who < 0` check
			// above.
			if (who)
				uid = who;
			else
				uid = geteuid();
			while (get_next_team_info(&team_cookie, &team) == B_OK) {
				if (team.uid != uid)
					continue;
				th_cookie = 0;
				while (get_next_thread_info(team.team, &th_cookie, &thread)
					== B_OK)
					if (thread.priority > out) {
						found = true;
						out = thread.priority;
					}
			}
			break;
		}
		default:
			errno = EINVAL;
			return -1;
	}
	if (!found) {
		errno = ESRCH;
		return -1;
	}
	return prio_be_to_unix(out) - NZERO;
}


int
setpriority(int which, id_t who, int value) {
	team_info team;
	thread_info thread;
	int32 team_cookie = 0, th_cookie = 0;
	bool found = false;

	if (who < 0) {
		errno = EINVAL;
		return -1;
	}
	value = CLIP_TO_UNIX(value + NZERO);
	value = prio_unix_to_be(value);

	switch (which) {
		// In Haiku process and process group are the same -- teams.
		case PRIO_PROCESS:
		case PRIO_PGRP:
		{
			status_t status;
			if (!who)
				who = getpid();
			status = get_team_info(who, &team);
			if (status == B_BAD_TEAM_ID) {
				errno = ESRCH;
				return -1;
			}
			while (get_next_thread_info(team.team, &th_cookie, &thread)
				== B_OK) {
				found = true;
				set_thread_priority(thread.thread, value);
			}
			break;
		}
		case PRIO_USER:
		{
			uid_t uid;
			// who (id_t) is signed, but uid_t is not, so have to use an
			// indirection like this
			if (who)
				uid = who;
			else
				uid = geteuid();
			while (get_next_team_info(&team_cookie, &team) == B_OK) {
				if (team.uid != uid)
					continue;
				th_cookie = 0;
				while (get_next_thread_info(team.team, &th_cookie, &thread)
					== B_OK) {
					found = true;
					set_thread_priority(thread.thread, value);
				}
			}
			break;
		}
		default:
			errno = EINVAL;
			return -1;
	}
	if (!found) {
		errno = ESRCH;
		return -1;
	}
	return 0;
}
