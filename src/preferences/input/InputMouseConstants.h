/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#ifndef INPUT_MOUSE_CONSTANTS_H
#define INPUT_MOUSE_CONSTANTS_H

#include "Input_Mouse_Touchpad.h"


const uint32 kMsgDefaults	        = 'BTde';
const uint32 kMsgRevert		        = 'BTre';
const uint32 kMsgDoubleClickSpeed	= 'SLdc';
const uint32 kMsgCursorSpeed	    = 'SLcs';
const uint32 kMsgFollowsMouseMode	= 'PUff';
const uint32 kMsgMouseFocusMode		= 'PUmf';
const uint32 kMsgAcceptFirstClick	= 'PUaf';
const uint32 kMsgMouseType			= 'PUmt';
const uint32 kMsgMouseMap			= 'PUmm';
const uint32 kMsgMouseSpeed			= 'SLms';
const uint32 kMsgAccelerationFactor	= 'SLma';

const uint32 kBorderSpace = 10;
const uint32 kItemSpace = 7;

#endif	/* INPUT_MOUSE_CONSTANTS_H */