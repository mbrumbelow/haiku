/*
 * Copyright 2021-2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ARCH_TRAPS_H_
#define _ARCH_TRAPS_H_


#include <SupportDefs.h>
#include <arch_thread_types.h>


extern "C" {

void SVec();
void STrap(iframe* frame);

};


void arch_traps_init();


#endif	// _ARCH_TRAPS_H_
