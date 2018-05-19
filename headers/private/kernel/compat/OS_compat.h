/*
 * Copyright 2018, Haiku Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_COMPAT_OS_H
#define _KERNEL_COMPAT_OS_H


// sizeof system_info x86		0x1c4
// sizeof system_info x86_64	0x1d0


typedef struct {
	bigtime_t		boot_time;			/* time of boot (usecs since 1/1/1970) */

	uint32			cpu_count;			/* number of cpus */

	uint64			max_pages;			/* total # of accessible pages */
	uint64			used_pages;			/* # of accessible pages in use */
	uint64			cached_pages;
	uint64			block_cache_pages;
	uint64			ignored_pages;		/* # of ignored/inaccessible pages */

	uint64			needed_memory;
	uint64			free_memory;

	uint64			max_swap_pages;
	uint64			free_swap_pages;

	uint32			page_faults;		/* # of page faults */

	uint32			max_sems;
	uint32			used_sems;

	uint32			max_ports;
	uint32			used_ports;

	uint32			max_threads;
	uint32			used_threads;

	uint32			max_teams;
	uint32			used_teams;

	char			kernel_name[B_FILE_NAME_LENGTH];
	char			kernel_build_date[B_OS_NAME_LENGTH];
	char			kernel_build_time[B_OS_NAME_LENGTH];

	int64			kernel_version;
	uint32			abi;				/* the system API */
} _PACKED compat_system_info;


#define compat_size_t uint32
#define compat_ptr_t uint32
typedef struct compat_area_info {
	area_id		area;
	char		name[B_OS_NAME_LENGTH];
	compat_size_t	size;
	uint32		lock;
	uint32		protection;
	team_id		team;
	uint32		ram_size;
	uint32		copy_count;
	uint32		in_count;
	uint32		out_count;
	compat_ptr_t	address;
} _PACKED compat_area_info;


// TODO: move to compat.cpp
// STATIC_ASSERT(sizeof(compat_area_info) == 0x48);


#endif // _KERNEL_COMPAT_OS_H
