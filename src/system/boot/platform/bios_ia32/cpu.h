/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef CPU_H
#define CPU_H


#include <SupportDefs.h>


#ifdef __cplusplus
extern "C" {
#endif

extern void cpu_init(void);

extern void
calculate_cpu_conversion_factor(uint8 channel);


#ifdef __cplusplus
}
#endif

#endif	/* CPU_H */
