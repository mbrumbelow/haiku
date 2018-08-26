/*
 * Copyright 2008-2010 Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BSD_PTY_H_
#define _BSD_PTY_H_


#ifdef _BSD_SOURCE


#include <termios.h>


#ifdef __cplusplus
extern "C" {
#endif

int		openpty(int* master, int* slave, char* name,
					struct termios* termAttrs, struct winsize* windowSize);
int		login_tty(int fd);
pid_t	forkpty(int* master, char* name,
					struct termios* termAttrs, struct winsize* windowSize);

#ifdef __cplusplus
}
#endif

#endif


#endif	// _BSD_PTY_H_
