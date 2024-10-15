/*
 * Copyright 2008, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2008, Philippe Saint-Pierre <stpere@gmail.com>
 * Copyright 2004-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef GENERIC_BLITTER_H
#define GENERIC_BLITTER_H


#include <SupportDefs.h>


struct BlitParameters {
	const uint8* from;
	uint32 fromWidth;
	uint16 fromLeft, fromTop;
	uint16 fromRight, fromBottom;

	uint8* to;
	uint32 toBytesPerRow;
	uint16 toLeft, toTop;
};


static void
blit8(const BlitParameters& params)
{
	const uint8* data = params.from;
	data += (params.fromWidth * params.fromTop + params.fromLeft);
	uint8* start = (uint8*)(params.to
		+ params.toBytesPerRow * params.toTop
		+ 1 * params.toLeft);

	for (int32 y = params.fromTop; y < params.fromBottom; y++) {
		const uint8* src = data;
		uint8* dst = start;
		for (int32 x = params.fromLeft; x < params.fromRight; x++) {
			dst[0] = src[0];
			dst++;
			src++;
		}

		data += params.fromWidth;
		start = (uint8*)((addr_t)start + params.toBytesPerRow);
	}
}


static void
blit15(const BlitParameters& params)
{
	const uint8* data = params.from;
	data += (params.fromWidth * params.fromTop + params.fromLeft) * 3;
	uint16* start = (uint16*)(params.to
		+ params.toBytesPerRow * params.toTop
		+ 2 * params.toLeft);

	for (int32 y = params.fromTop; y < params.fromBottom; y++) {
		const uint8* src = data;
		uint16* dst = start;
		for (int32 x = params.fromLeft; x < params.fromRight; x++) {
			dst[0] = ((src[2] >> 3) << 10)
				| ((src[1] >> 3) << 5)
				| ((src[0] >> 3));

			dst++;
			src += 3;
		}

		data += params.fromWidth * 3;
		start = (uint16*)((addr_t)start + params.toBytesPerRow);
	}
}


static void
blit16(const BlitParameters& params)
{
	const uint8* data = params.from;
	data += (params.fromWidth * params.fromTop + params.fromLeft) * 3;
	uint16* start = (uint16*)(params.to
		+ params.toBytesPerRow * params.toTop
		+ 2 * params.toLeft);

	for (int32 y = params.fromTop; y < params.fromBottom; y++) {
		const uint8* src = data;
		uint16* dst = start;
		for (int32 x = params.fromLeft; x < params.fromRight; x++) {
			dst[0] = ((src[2] >> 3) << 11)
				| ((src[1] >> 2) << 5)
				| ((src[0] >> 3));

			dst++;
			src += 3;
		}

		data += params.fromWidth * 3;
		start = (uint16*)((addr_t)start + params.toBytesPerRow);
	}
}


static void
blit24(const BlitParameters& params)
{
	const uint8* data = params.from;
	data += (params.fromWidth * params.fromTop + params.fromLeft) * 3;
	uint8* start = (uint8*)(params.to
		+ params.toBytesPerRow * params.toTop
		+ 3 * params.toLeft);

	for (int32 y = params.fromTop; y < params.fromBottom; y++) {
		const uint8* src = data;
		uint8* dst = start;
		for (int32 x = params.fromLeft; x < params.fromRight; x++) {
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			dst += 3;
			src += 3;
		}

		data += params.fromWidth * 3;
		start = (uint8*)((addr_t)start + params.toBytesPerRow);
	}
}


static void
blit32(const BlitParameters& params)
{
	const uint8* data = params.from;
	data += (params.fromWidth * params.fromTop + params.fromLeft) * 3;
	uint32* start = (uint32*)(params.to
		+ params.toBytesPerRow * params.toTop
		+ 4 * params.toLeft);

	for (int32 y = params.fromTop; y < params.fromBottom; y++) {
		const uint8* src = data;
		uint32* dst = start;
		for (int32 x = params.fromLeft; x < params.fromRight; x++) {
			dst[0] = (src[2] << 16) | (src[1] << 8) | (src[0]);
			dst++;
			src += 3;
		}

		data += params.fromWidth * 3;
		start = (uint32*)((addr_t)start + params.toBytesPerRow);
	}
}


static void
blit(const BlitParameters& params, int32 depth)
{
	switch (depth) {
		case 8:
			blit8(params);
			return;
		case 15:
			blit15(params);
			return;
		case 16:
			blit16(params);
			return;
		case 24:
			blit24(params);
			return;
		case 32:
			blit32(params);
			return;
	}
}


// #pragma mark - scaled blitting


template<uint32 bytesPerPixel>
struct BlitScaledImage {
	uint8* data;
	uint32 bytes_per_row;
	struct {
		uint16 x, y;
	} offset;

	uint32 getPixel(uint32 x, uint32 y) const
	{
		x += offset.x;
		y += offset.y;
		uint8* pixel = &data[(y * bytes_per_row) + x * bytesPerPixel];
		if (bytesPerPixel == 4)
			return *(uint32*)pixel;
		else if (bytesPerPixel == 3)
			return (pixel[2] << 16) | (pixel[1] << 8) | (pixel[0]);
		return 0;
	}
	void setPixel(uint32 x, uint32 y, uint32 value)
	{
		x += offset.x;
		y += offset.y;
		*(uint32*)(&data[(y * bytes_per_row) + (x * bytesPerPixel)]) = value;
	}
};


static inline float
interpolate_linear(float s, float e, float t)
{
	return s + (e - s) * t;
}


static inline float
interpolate_bilinear(float c00, float c10, float c01, float c11, float tx, float ty)
{
	return interpolate_linear(interpolate_linear(c00, c10, tx),
		interpolate_linear(c01, c11, tx), ty);
}


static void
blit32_scaled(const BlitParameters& params, float scale)
{
	const BlitScaledImage<3> source = {(uint8*)params.from, uint32(params.fromWidth) * 3,
		params.fromLeft, params.fromTop};
	BlitScaledImage<4> dest = {(uint8*)params.to, (uint32)params.toBytesPerRow,
		params.toLeft, params.toTop};

	const uint16 sourceWidth = (params.fromRight - params.fromLeft);
	const uint16 sourceHeight = (params.fromBottom - params.fromTop);
	const uint16 newWidth = sourceWidth * scale;
	const uint16 newHeight = sourceHeight * scale;
	for (uint32 x = 0; x < newWidth; x++) {
		for (uint32 y = 0; y < newHeight; y++) {
			const float gx = ((float)x) / newWidth * (sourceWidth - 1);
			const float gy = ((float)y) / newHeight * (sourceHeight - 1);
			const uint32 gxi = (uint32)gx, gyi = (uint32)gy;

			union {
				uint32 value;
				uint8 byte(uint8 index) { return (value >> (index * 8)) & 0xFF; }
			} c00, c10, c01, c11;
			c00.value = source.getPixel(gxi, gyi);
			c10.value = source.getPixel(gxi + 1, gyi);
			c01.value = source.getPixel(gxi, gyi + 1);
			c11.value = source.getPixel(gxi + 1, gyi + 1);
			uint32 rgb = 0;
			for (uint8 i = 0; i < 3; i++) {
				const uint32 value = interpolate_bilinear(
					c00.byte(i), c10.byte(i), c01.byte(i), c11.byte(i),
					gx - gxi, gy - gyi);
				rgb |= (value << (i * 8));
			}
			dest.setPixel(x, y, rgb);
		}
	}
}


#endif	/* GENERIC_BLITTER_H */
