/* 
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Copyright 2004, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "WriterPlugin.h"

#include <stdio.h>


Writer::Writer()
	:
	fTarget(NULL),
	fMediaPlugin(NULL)
{
}


Writer::~Writer()
{
}


BDataIO*
Writer::Target() const
{
	return fTarget;
}


void
Writer::_Setup(BDataIO* target)
{
	fTarget = target;
}


void Writer::_ReservedWriter1() {}
void Writer::_ReservedWriter2() {}
void Writer::_ReservedWriter3() {}
void Writer::_ReservedWriter4() {}
void Writer::_ReservedWriter5() {}
