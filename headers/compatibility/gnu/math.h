/*
 * Copyright 2018 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _GNU_MATH_H_
#define _GNU_MATH_H_


#include_next <math.h>


#ifdef _GNU_SOURCE


#ifdef __cplusplus
extern "C" {
#endif

extern void sincos(double, double *, double *);
extern void sincosf(float, float *, float *);
extern void sincosl(long double, long double *, long double *);

#ifdef __cplusplus
}
#endif


#endif


#endif	/* _GNU_MATH_H_ */
