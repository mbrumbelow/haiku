/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HSP_H
#define _HSP_H


#include <GraphicsDefs.h>


typedef struct hsp_color {
	float	hue;
	float	saturation;
	float	brightness;

	hsp_color set_to(float h, float s, float b);

	inline bool	IsDark() const;

	inline bool IsLight() const;

	static inline float Contrast(hsp_color colorA, hsp_color colorB);

	static	hsp_color	from_rgb(const rgb_color& color);
	static	hsp_color	from_float(float red, float green, float blue);
			rgb_color	to_rgb() const;

} hsp_color;


#endif // _HSP_H
