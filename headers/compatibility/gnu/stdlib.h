/*
 * Copyright 2020 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _GNU_STDLIB_H_
#define _GNU_STDLIB_H_


#include_next <stdlib.h>


#ifdef _GNU_SOURCE


#ifdef __cplusplus
extern "C" {
#endif

typedef int (*_compare_function_reentrant)(const void *, const void *, void *);

extern void		qsort_r(void *base, size_t numElements, size_t sizeOfElement,
					_compare_function_reentrant, void *cookie);

#ifdef __cplusplus
}
#endif


#endif


#endif	/* _GNU_PTHREAD_H_ */
