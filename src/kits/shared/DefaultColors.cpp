/*
 * Copyright 2022, Pascal Abresch, nep@packageloss.eu.
 * Distributed under the terms of the MIT License.
 */

#include "DefaultColors.h"
#include <private/interface/DefaultColors.h>
#include <private/app/ServerReadOnlyMemory.h>


namespace BPrivate {

rgb_color GetSystemColor(color_which colorConstant, bool darkVariant) {
	if (darkVariant) {
		return BPrivate::kDefaultColorsDark[color_which_to_index(colorConstant)];
	} else {
		return BPrivate::kDefaultColors[color_which_to_index(colorConstant)];
	}
}

} // namespace BPrivate
