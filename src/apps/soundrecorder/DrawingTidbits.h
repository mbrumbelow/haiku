/*
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Inspired by SoundCapture from Be newsletter (Media Kit Basics: Consumers and Producers)
 */
 
#ifndef __DRAWING_TIBITS__
#define __DRAWING_TIBITS__

#include <GraphicsDefs.h>

rgb_color ShiftColor(rgb_color , float );

bool operator==(const rgb_color &, const rgb_color &);
bool operator!=(const rgb_color &, const rgb_color &);

inline uchar
ShiftComponent(uchar component, float percent)
{
	// change the color by <percent>, make sure we aren't rounding
	// off significant bits
	if (percent >= 1)
		return (uchar)(component * (2 - percent));
	else
		return (uchar)(255 - percent * (255 - component));
}


const float kDarkness = 1.06;
const float kDimLevel = 0.6;

void ReplaceColor(BBitmap *bitmap, rgb_color from, rgb_color to);
void ReplaceTransparentColor(BBitmap *bitmap, rgb_color with);

#endif

