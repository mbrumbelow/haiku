/*
 * Copyright 2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef MUSL_HAIKU_H
#define MUSL_HAIKU_H

/*! Note that this file is also included as part of the kernel C library! */

#include <BeBuild.h>

#define weak_alias(old, new) B_DEFINE_WEAK_ALIAS(old, new)

#if __GNUC__ < 3
#define restrict __restrict
#endif

#endif
