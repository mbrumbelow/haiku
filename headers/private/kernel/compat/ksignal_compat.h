/*
 * Copyright 2018, Haiku Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_COMPAT_KSIGNAL_H
#define _KERNEL_COMPAT_KSIGNAL_H


#include <compat/signal_compat.h>


/*

x86_64:
sizeof signal_frame_data 824
sizeof siginfo_t 56
sizeof ucontext_t 696
sizeof sigset_t 8
sizeof stack_t 24
sizeof mcontext_t 656


x86
sizeof signal_frame_data 680
sizeof siginfo_t 36
sizeof ucontext_t 584
sizeof sigset_t 8
sizeof stack_t 12
sizeof mcontext_t 560

*/


typedef struct packed_fp_stack {
	unsigned char	st0[10];
	unsigned char	st1[10];
	unsigned char	st2[10];
	unsigned char	st3[10];
	unsigned char	st4[10];
	unsigned char	st5[10];
	unsigned char	st6[10];
	unsigned char	st7[10];
} packed_fp_stack;

typedef struct packed_mmx_regs {
	unsigned char	mm0[10];
	unsigned char	mm1[10];
	unsigned char	mm2[10];
	unsigned char	mm3[10];
	unsigned char	mm4[10];
	unsigned char	mm5[10];
	unsigned char	mm6[10];
	unsigned char	mm7[10];
} packed_mmx_regs;

typedef struct xmmx_regs {
	unsigned char	xmm0[16];
	unsigned char	xmm1[16];
	unsigned char	xmm2[16];
	unsigned char	xmm3[16];
	unsigned char	xmm4[16];
	unsigned char	xmm5[16];
	unsigned char	xmm6[16];
	unsigned char	xmm7[16];
} xmmx_regs;

typedef struct compat_stack_t {
	uint32	ss_sp;
	uint32	ss_size;
	int		ss_flags;
} compat_stack_t;

typedef struct compat_old_extended_regs {
	unsigned short	fp_control;
	unsigned short	_reserved1;
	unsigned short	fp_status;
	unsigned short	_reserved2;
	unsigned short	fp_tag;
	unsigned short	_reserved3;
	uint32	fp_eip;
	unsigned short	fp_cs;
	unsigned short	fp_opcode;
	uint32	fp_datap;
	unsigned short	fp_ds;
	unsigned short	_reserved4;
	union {
		packed_fp_stack	fp;
		packed_mmx_regs	mmx;
	} fp_mmx;
} compat_old_extended_regs;

typedef struct compat_new_extended_regs {
	unsigned short	fp_control;
	unsigned short	fp_status;
	unsigned short	fp_tag;
	unsigned short	fp_opcode;
	uint32	fp_eip;
	unsigned short	fp_cs;
	unsigned short	res_14_15;
	uint32	fp_datap;
	unsigned short	fp_ds;
	unsigned short	_reserved_22_23;
	uint32	mxcsr;
	uint32	_reserved_28_31;
	union {
		fp_stack fp;
		mmx_regs mmx;
	} fp_mmx;
	xmmx_regs xmmx;
	unsigned char	_reserved_288_511[224];
} compat_new_extended_regs;

typedef struct compat_extended_regs {
	union {
		compat_old_extended_regs	old_format;
		compat_new_extended_regs	new_format;
	} state;
	uint32	format;
} compat_extended_regs;

struct compat_vregs {
	uint32			eip;
	uint32			eflags;
	uint32			eax;
	uint32			ecx;
	uint32			edx;
	uint32			esp;
	uint32			ebp;
	uint32			_reserved_1;
	compat_extended_regs	xregs;
	uint32			edi;
	uint32			esi;
	uint32			ebx;
};

typedef struct compat_vregs compat_mcontext_t;

typedef struct __compat_ucontext_t {
	uint32					uc_link;
	sigset_t				uc_sigmask;
	compat_stack_t			uc_stack;
	compat_mcontext_t		uc_mcontext;
} _PACKED compat_ucontext_t;

union compat_sigval {
	int		sival_int;
	uint32	sival_ptr;
};


typedef struct __compat_siginfo_t {
	int				si_signo;	/* signal number */
	int				si_code;	/* signal code */
	int				si_errno;	/* if non zero, an error number associated with
								   this signal */
	pid_t			si_pid;		/* sending process ID */
	uid_t			si_uid;		/* real user ID of sending process */
	uint32			si_addr;	/* address of faulting instruction */
	int				si_status;	/* exit value or signal */
	uint32			si_band;	/* band event for SIGPOLL */
	union compat_sigval	si_value;	/* signal value */
} compat_siginfo_t;

struct compat_signal_frame_data {
	compat_siginfo_t	info;
	compat_ucontext_t	context;
	uint32		user_data;
	uint32		handler;
	bool		siginfo_handler;
	char		_pad[3];
	int32		thread_flags;
	uint64		syscall_restart_return_value;
	uint8		syscall_restart_parameters[SYSCALL_RESTART_PARAMETER_SIZE];
	uint32		commpage_address;
} _PACKED;


#endif // _KERNEL_COMPAT_KSIGNAL_H
