/*
 * Copyright 2018, Haiku Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_COMPAT_IMAGE_H
#define _KERNEL_COMPAT_IMAGE_H


#include <image.h>


#define compat_uintptr_t uint32
typedef struct {
	image_id	id;
	image_type	type;
	int32		sequence;
	int32		init_order;
	compat_uintptr_t	init_routine;
	compat_uintptr_t	term_routine;
	dev_t		device;
	ino_t		node;
	char		name[MAXPATHLEN];
	compat_uintptr_t	text;
	compat_uintptr_t	data;
	int32		text_size;
	int32		data_size;

	/* Haiku R1 extensions */
	int32		api_version;	/* the Haiku API version used by the image */
	int32		abi;			/* the Haiku ABI used by the image */
} _PACKED compat_image_info;


typedef struct {
	compat_image_info	basic_info;
	int32		text_delta;
	compat_uintptr_t	symbol_table;
	compat_uintptr_t	symbol_hash;
	compat_uintptr_t	string_table;
} _PACKED compat_extended_image_info;


STATIC_ASSERT(sizeof(compat_image_info) == 1084);
STATIC_ASSERT(sizeof(compat_extended_image_info) == 1100);


#endif // _KERNEL_COMPAT_IMAGE_H
