/*
 * Copyright 2019-2020, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef DTB_H
#define DTB_H

#ifndef _ASSEMBLER

#include "efi_platform.h"

#include <util/FixedWidthPointer.h>


#ifdef __cplusplus
extern "C" {
#endif

extern void dtb_init();

#ifdef __cplusplus
}
#endif


#endif /* !_ASSEMBLER */

#endif /* DTB_H */
