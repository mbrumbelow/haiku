/*
 * Copyright 2001-2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef DEBUG_H
#define DEBUG_H


#include "system_dependencies.h"

#ifdef USER
#	define __out printf
#else
#	define __out dprintf
#endif

// Which debugger should be used when?
// The DEBUGGER() macro actually has no effect if DEBUG is not defined,
// use the DIE() macro if you really want to die.
#ifdef DEBUG
#	ifdef USER
#		define DEBUGGER(x) debugger x
#	else
#		define DEBUGGER(x) kernel_debugger x
#	endif
#else
#	define DEBUGGER(x) ;
#endif

#ifdef USER
#	define DIE(x) debugger x
#else
#	define DIE(x) kernel_debugger x
#endif

// Short overview over the debug output macros:
//	PRINT()
//		is for general messages that very unlikely should appear in a release build
//	FATAL()
//		this is for fatal messages, when something has really gone wrong
//	INFORM()
//		general information, as disk size, etc.
//	REPORT_ERROR(status_t)
//		prints out error information
//	RETURN_ERROR(status_t)
//		calls REPORT_ERROR() and return the value
//	D()
//		the statements in D() are only included if DEBUG is defined

#if 0//DEBUG
	#define PRINT(x) { __out("btrfs: "); __out x; }
	#define REPORT_ERROR(status) \
		__out("btrfs: %s:%d: %s\n", __FUNCTION__, __LINE__, strerror(status));
	#define RETURN_ERROR(err) { status_t _status = err; if (_status < B_OK) REPORT_ERROR(_status); return _status;}
	#define FATAL(x) { __out("btrfs: "); __out x; }
	#define INFORM(x) { __out("btrfs: "); __out x; }
//	#define FUNCTION() __out("btrfs: %s()\n",__FUNCTION__);
	#define FUNCTION_START(x) { __out("btrfs: %s() ",__FUNCTION__); __out x; }
	#define FUNCTION() ;
//	#define FUNCTION_START(x) ;
	#define D(x) {x;};
	#ifndef ASSERT
	#	define ASSERT(x) { if (!(x)) DEBUGGER(("btrfs: assert failed: " #x "\n")); }
	#endif
#else
	#define PRINT(x) ;
	#define REPORT_ERROR(status) \
		__out("btrfs: %s:%d: %s\n", __FUNCTION__, __LINE__, strerror(status));
	#define RETURN_ERROR(err) { status_t _status = err; if (_status < B_OK) REPORT_ERROR(_status); return _status;}
//	#define FATAL(x) { panic x; }
	#define FATAL(x) { __out("btrfs: "); __out x; }
	#define INFORM(x) { __out("btrfs: "); __out x; }
	#define FUNCTION() ;
	#define FUNCTION_START(x) ;
	#define D(x) ;
	#ifndef ASSERT
	#	define ASSERT(x) { if (!(x)) DEBUGGER(("btrfs: assert failed: " #x "\n")); }
//	#	define ASSERT(x) ;
	#endif
#endif

#endif	/* DEBUG_H */
