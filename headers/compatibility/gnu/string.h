/*
 * Copyright 2018 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _GNU_STRING_H_
#define _GNU_STRING_H_


#include_next <string.h>


#ifdef _GNU_SOURCE


#ifdef __cplusplus
extern "C" {
#endif

void *memmem(const void *l, size_t l_len, const void *s, size_t s_len);

#ifdef __cplusplus
}
#endif


#endif


#endif	/* _GNU_STRING_H_ */
