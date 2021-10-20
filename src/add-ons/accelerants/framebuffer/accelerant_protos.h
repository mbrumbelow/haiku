/*
 * Copyright 2005-2008, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2016, Jessica Hamilton, jessica.l.hamilton@gmail.com.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _ACCELERANT_PROTOS_H
#define _ACCELERANT_PROTOS_H


#include <Accelerant.h>
#include "video_overlay.h"


#ifdef __cplusplus
extern "C" {
#endif

// general
status_t framebuffer_init_accelerant(int fd);
ssize_t framebuffer_accelerant_clone_info_size(void);
void framebuffer_get_accelerant_clone_info(void *data);
status_t framebuffer_clone_accelerant(void *data);
void framebuffer_uninit_accelerant(void);
status_t framebuffer_get_accelerant_device_info(accelerant_device_info *adi);
sem_id framebuffer_accelerant_retrace_semaphore(void);

// modes & constraints
uint32 framebuffer_accelerant_mode_count(void);
status_t framebuffer_get_mode_list(display_mode *dm);
status_t framebuffer_set_display_mode(display_mode *modeToSet);
status_t framebuffer_get_display_mode(display_mode *currentMode);
status_t framebuffer_get_frame_buffer_config(frame_buffer_config *config);
status_t framebuffer_get_pixel_clock_limits(display_mode *dm, uint32 *low,
	uint32 *high);

// accelerant engine
uint32 framebuffer_accelerant_engine_count(void);
status_t framebuffer_acquire_engine(uint32 capabilities, uint32 maxWait,
	sync_token *st, engine_token **et);
status_t framebuffer_release_engine(engine_token *et, sync_token *st);
void framebuffer_wait_engine_idle(void);
status_t framebuffer_get_sync_token(engine_token *et, sync_token *st);
status_t framebuffer_sync_to_token(sync_token *st);

#ifdef __cplusplus
}
#endif

#endif	/* _ACCELERANT_PROTOS_H */
