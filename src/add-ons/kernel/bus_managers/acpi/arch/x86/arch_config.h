/*
 * Copyright 2022 Adrien Destugues <pulkomandy@pulkomandy.tk>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef ARCH_CONFIG_H
#define ARCH_CONFIG_H


#define ACPI_FLUSH_CPU_CACHE() __asm __volatile("wbinvd");


#endif /* !ARCH_CONFIG_H */
