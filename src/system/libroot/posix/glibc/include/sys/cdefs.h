#ifndef _GLIBC_SYS_CDEFS
#define _GLIBC_SYS_CDEFS

#include <libc-symbols.h>
#include <misc/sys/cdefs.h>

#if (__GNUC__ >= 3)
# define __glibc_unlikely(cond)	__builtin_expect ((cond), 0)
# define __glibc_likely(cond)	__builtin_expect ((cond), 1)
#else
# define __glibc_unlikely(cond)	(cond)
# define __glibc_likely(cond)	(cond)
#endif

#endif /* _GLIBC_SYS_CDEFS */
