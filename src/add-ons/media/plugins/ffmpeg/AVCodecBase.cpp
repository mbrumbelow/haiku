/*
 * Copyright 2018, Haiku, Inc.
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "AVCodecBase.h"

#include <new>

#include <stdio.h>
#include <string.h>


#undef TRACE
//#define TRACE_AV_CODEC_BASE
#ifdef TRACE_AV_CODEC_BASE
#	define TRACE	printf
#	define TRACE_IO(a...)
#else
#	define TRACE(a...)
#	define TRACE_IO(a...)
#endif



AVCodecBase::AVCodecBase()
	:
	fCodecInitStatus(CODEC_INIT_NEEDED)
{
	TRACE("AVCodecBase::AVCodecBase()\n");
}


AVCodecBase::~AVCodecBase()
{
	TRACE("AVCodecBase::~AVCodecBase()\n");
}

