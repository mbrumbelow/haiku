/*
 * Copyright 2023 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BSD_TERMIOS_H_
#define _BSD_TERMIOS_H_


#include <features.h>

#include_next <termios.h>


#ifdef _DEFAULT_SOURCE
extern int cfsetspeed(struct termios *termios, speed_t speed);
#endif


#endif
