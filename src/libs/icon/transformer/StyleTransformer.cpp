/*
 * Copyright 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */

#include "StyleTransformer.h"

#include <Point.h>


_USING_ICON_NAMESPACE


StyleTransformer::~StyleTransformer()
{
}


void
StyleTransformer::transform(BPoint* point) const
{
	if (point) {
		double x = point->x;
		double y = point->y;

		transform(&x, &y);

		point->x = x;
		point->y = y;
	}
}


BPoint
StyleTransformer::transform(const BPoint& point) const
{
	BPoint p(point);
	transform(&p);
	return p;
}
