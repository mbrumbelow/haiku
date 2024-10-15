/*
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef GENERIC_VIDEO_SPLASH_H
#define GENERIC_VIDEO_SPLASH_H


#include <SupportDefs.h>


void
compute_splash_logo_placement(uint32 screenWidth, uint32 screenHeight, uint8 scale,
	int& baseWidth, int& baseHeight, int& x, int& y)
{
	uint16 iconsHalfHeight = kSplashIconsHeight / 2;

	baseWidth = min_c(kSplashLogoWidth, screenWidth);
	int outputWidth = min_c(uint32(kSplashLogoWidth * scale), screenWidth);
	int outputHeight = min_c(uint32(kSplashLogoHeight + iconsHalfHeight) * scale, screenHeight);
	int placementX = max_c(0, min_c(100, kSplashLogoPlacementX));
	int placementY = max_c(0, min_c(100, kSplashLogoPlacementY));

	x = (screenHeight - outputWidth) * placementX / 100;
	y = (screenHeight - outputHeight) * placementY / 100;

	baseHeight = min_c(kSplashLogoHeight, screenHeight);
}


void
compute_splash_icons_placement(uint32 screenWidth, uint32 screenHeight, uint8 scale,
	int& baseWidth, int& baseHeight, int& x, int& y)
{
	uint16 iconsHalfHeight = kSplashIconsHeight / 2;

	baseWidth = min_c(kSplashIconsWidth, screenWidth);
	int outputWidth = min_c(uint32(kSplashIconsWidth * scale), screenWidth);
	int outputHeight = min_c(uint32(kSplashLogoHeight + iconsHalfHeight) * scale, screenHeight);
	int placementX = max_c(0, min_c(100, kSplashIconsPlacementX));
	int placementY = max_c(0, min_c(100, kSplashIconsPlacementY));

	x = (screenWidth - outputWidth) * placementX / 100;
	y = (kSplashLogoHeight * scale)
		+ (screenHeight - outputHeight) * placementY / 100;

	baseHeight = min_c(iconsHalfHeight, screenHeight);
}


#endif	/* GENERIC_VIDEO_SPLASH_H */
