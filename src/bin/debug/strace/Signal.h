/*
 * Copyright 2023, Trung Nguyen, trungnt282910@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef STRACE_SIGNAL_H
#define STRACE_SIGNAL_H


#include <signal.h>


const char *signal_name(int signal);
const char *signal_info(siginfo_t& info);


#endif	// STRACE_SIGNAL_H
