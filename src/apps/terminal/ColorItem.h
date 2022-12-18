/*
 * Copyright 2001-2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm, bpmagic@columbus.rr.com
 *		Rene Gollent, rene@gollent.com
 *		Ryan Leavengood, leavengood@gmail.com
 *		John Scipione, jscipione@gmail.com
 */
#ifndef COLOR_ITEM_H
#define COLOR_ITEM_H


#include <InterfaceDefs.h>
#include <StringItem.h>


class ColorItem : public BStringItem
{
public:
							ColorItem(const char* text, rgb_color color);

	virtual void			DrawItem(BView* owner, BRect frame, bool complete);

			rgb_color		Color() { return fColor; };
			void			SetColor(rgb_color color) { fColor = color; };

private:
			rgb_color		fColor;
};


#endif
