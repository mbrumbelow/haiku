/*
 * Copyright 2006, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DEFAULT_COLORS_H
#define _DEFAULT_COLORS_H


#include <GraphicsDefs.h>
#include <InterfaceDefs.h>


namespace BPrivate {

extern const rgb_color* kDefaultColors;
extern const rgb_color* kDefaultColorsDark;

rgb_color GetSystemColor(color_which, bool darkVariant);
}	// namespace BPrivate

#endif // _DEFAULT_COLORS_H
