/*
 * Copyright 2017-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef LOGGER_H
#define LOGGER_H

#include <String.h>
#include <File.h>
#include <Path.h>

#include <ctype.h>
#include <stdio.h>


// These macros allow for standardized logging to be output.  Depending on the
// number of arguments to your "printf" format, you will use the appropriate
// macro; for example HDINFO0 would be used for no arguments and HDINFO3 would
// be used for a case where there is three arguments to the format.
// The use of macros in this way means that the use of the log is concise where
// it is used and also because the macro unwraps to a block contained with a
// condition statement, if the log level is not sufficient to trigger the log
// line then there is no computational cost to running over the log space.  This
// is because the arguments will not be evaluated.  Avoiding all of the
// conditional clauses in the code to prevent this otherwise would be
// cumbersome.

#define HDLOGPREFIX(L) printf("{%c} ", toupper(Logger::NameForLevel(L)[0]));

#define HDLOG0(L, M) if (Logger::IsLevelEnabled(L)) { \
	printf("%c| %s\n", Logger::NameForLevel(L)[0], M); \
}
#define HDLOG1(L, M, A0) if (Logger::IsLevelEnabled(L)) { \
	HDLOGPREFIX(L); \
	printf(M, A0); \
	printf("\n"); \
}
#define HDLOG2(L, M, A0, A1) if (Logger::IsLevelEnabled(L)) { \
	HDLOGPREFIX(L); \
	printf(M, A0, A1); \
	printf("\n"); \
}
#define HDLOG3(L, M, A0, A1, A2) if (Logger::IsLevelEnabled(L)) { \
	HDLOGPREFIX(L); \
	printf(M, A0, A1, A2); \
	printf("\n"); \
}
#define HDLOG4(L, M, A0, A1, A2, A3) if (Logger::IsLevelEnabled(L)) { \
	HDLOGPREFIX(L); \
	printf(M, A0, A1, A2, A3); \
	printf("\n"); \
}

#define HDINFO0(M) HDLOG0(LOG_LEVEL_INFO, M)
#define HDINFO1(M, A0) HDLOG1(LOG_LEVEL_INFO, M, A0)
#define HDINFO2(M, A0, A1) HDLOG2(LOG_LEVEL_INFO, M, A0, A1)
#define HDINFO3(M, A0, A1, A2) HDLOG3(LOG_LEVEL_INFO, M, A0, A1, A2)

#define HDDEBUG0(M) HDLOG0(LOG_LEVEL_DEBUG, M)
#define HDDEBUG1(M, A0) HDLOG1(LOG_LEVEL_DEBUG, M, A0)
#define HDDEBUG2(M, A0, A1) HDLOG2(LOG_LEVEL_DEBUG, M, A0, A1)
#define HDDEBUG3(M, A0, A1, A2) HDLOG3(LOG_LEVEL_DEBUG, M, A0, A1, A2)
#define HDDEBUG4(M, A0, A1, A2, A3) HDLOG4(LOG_LEVEL_DEBUG, M, A0, A1, A2, A3)

#define HDTRACE0(M) HDLOG0(LOG_LEVEL_TRACE, M)
#define HDTRACE1(M, A0) HDLOG1(LOG_LEVEL_TRACE, M, A0)
#define HDTRACE2(M, A0, A1) HDLOG2(LOG_LEVEL_TRACE, M, A0, A1)
#define HDTRACE3(M, A0, A1, A2) HDLOG3(LOG_LEVEL_TRACE, M, A0, A1, A2)

#define HDERROR0(M) HDLOG0(LOG_LEVEL_ERROR, M)
#define HDERROR1(M, A0) HDLOG1(LOG_LEVEL_ERROR, M, A0)
#define HDERROR2(M, A0, A1) HDLOG2(LOG_LEVEL_ERROR, M, A0, A1)
#define HDERROR3(M, A0, A1, A2) HDLOG3(LOG_LEVEL_ERROR, M, A0, A1, A2)


typedef enum log_level {
	LOG_LEVEL_OFF		= 1,
	LOG_LEVEL_ERROR		= 2,
	LOG_LEVEL_INFO		= 3,
	LOG_LEVEL_DEBUG		= 4,
	LOG_LEVEL_TRACE		= 5
} log_level;


class Logger {
public:
	static	log_level			Level();
	static	void				SetLevel(log_level value);
	static	bool				SetLevelByName(const char *name);

	static	const char*			NameForLevel(log_level value);

	static	bool				IsLevelEnabled(log_level value);
	static	bool				IsInfoEnabled();
	static	bool				IsDebugEnabled();
	static	bool				IsTraceEnabled();

private:
	static	log_level			fLevel;
};


#endif // LOGGER_H
