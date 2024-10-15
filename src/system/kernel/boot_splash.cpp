/*
 * Copyright 2008-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Artur Wyszynski <harakash@gmail.com>
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <KernelExport.h>

#define __BOOTSPLASH_KERNEL__
#include <boot/images.h>
#include <boot/platform/generic/video_blitter.h>
#include <boot/platform/generic/video_splash.h>

#include <boot_item.h>
#include <debug.h>
#include <frame_buffer_console.h>

#include <boot_splash.h>


//#define TRACE_BOOT_SPLASH 1
#ifdef TRACE_BOOT_SPLASH
#	define TRACE(x...) dprintf(x);
#else
#	define TRACE(x...) ;
#endif


static struct frame_buffer_boot_info *sInfo;
static uint8 *sUncompressedIcons;


//	#pragma mark - exported functions


void
boot_splash_init(uint8 *bootSplash)
{
	TRACE("boot_splash_init: enter\n");

	if (debug_screen_output_enabled())
		return;

	sInfo = (frame_buffer_boot_info *)get_boot_item(FRAME_BUFFER_BOOT_INFO,
		NULL);

	sUncompressedIcons = bootSplash;
}


void
boot_splash_uninit(void)
{
	sInfo = NULL;
}


void
boot_splash_set_stage(int stage)
{
	TRACE("boot_splash_set_stage: stage=%d\n", stage);

	if (sInfo == NULL || stage < 0 || stage >= BOOT_SPLASH_STAGE_MAX)
		return;

	uint8 scale = 1;
	if (sInfo->width > 1920 && sInfo->height > 1080 && sInfo->depth == 32)
		scale = 2;

	int baseWidth, baseHeight, x, y;
	compute_splash_icons_placement(sInfo->width, sInfo->height,
		scale, baseWidth, baseHeight, x, y);

	int stageLeftEdge = baseWidth * stage / BOOT_SPLASH_STAGE_MAX;
	int stageRightEdge = baseWidth * (stage + 1) / BOOT_SPLASH_STAGE_MAX;

	BlitParameters params;
	params.from = sUncompressedIcons;
	params.fromWidth = kSplashIconsWidth;
	params.fromLeft = stageLeftEdge;
	params.fromTop = 0;
	params.fromRight = stageRightEdge;
	params.fromBottom = baseHeight;
	params.to = (uint8*)sInfo->frame_buffer;
	params.toBytesPerRow = sInfo->bytes_per_row;
	params.toLeft = (stageLeftEdge * scale) + x;
	params.toTop = y;

	if (scale > 1) {
		blit32_scaled(params, scale);
	} else {
		blit(params, sInfo->depth);
	}
}
