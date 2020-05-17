/*
 * Copyright 2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __INPUT_ICONS_H
#define __INPUT_ICONS_H

#include <Bitmap.h>


class BResources;


struct InputIcons {
								InputIcons();

	struct IconSet {
								IconSet();

	};

			BBitmap				mouseIcon;
			BBitmap				touchpadIcon;
			BBitmap				keyboardIcon;

	static	BRect				IconRectAt(const BPoint& topLeft);

	static	const BRect			sBounds;

private:

			void				_LoadBitmap(BResources* resources, int32 id,
									BBitmap* bitmap);
};

#endif

