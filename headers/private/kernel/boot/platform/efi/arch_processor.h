/*
 * Copyright 2013-2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef EFI_BOOT_PLATFORM_EFI_ARCH_PROCESSOR_H
#define EFI_BOOT_PLATFORM_EFI_ARCH_PROCESSOR_H


#include <SupportDefs.h>


#ifdef __cplusplus
extern "C" {
#endif

status_t arch_processor_init(void);
void arch_ucode_load(BootVolume& volume);

#ifdef __cplusplus
}
#endif


#endif /* EFI_BOOT_PLATFORM_EFI_ARCH_PROCESSOR_H */
