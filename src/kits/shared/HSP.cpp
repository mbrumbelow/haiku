/*
 * Copyright 2025 Pascal Abresch. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <HSP.h>

#include <math.h>

hsp_color
hsp_color::from_rgb(const rgb_color& rgbcolor)
{
	hsp_color result;

	float red =  rgbcolor.red / 255.0f;
	float green = rgbcolor.green / 255.0f;
	float blue = rgbcolor.blue / 255.0f;

	float hue, saturation;

	result.brightness = sqrtf(0.299f * red * red + 0.587f * green * green + 0.114 * blue * blue);

	if (red == green && red == blue) {
		result.hue = 0;
		result.saturation = 0;

		return result;
	}

	if (red >= green && red >= blue)
	{
		if (blue >= green) {
			hue = (6/6) - (1/6) * (blue - green) / (red - green);
			saturation = 1 - (green / red);
		} else {
			hue = (0/6) + (1/6) * (green - blue) / (green - blue);
			saturation = 1 - (green / red);
		}
	} else if (green >= red && green <= blue) {
		if (red >= blue) {
			hue = (2/6) - (1/6) * (red - blue) / (green - blue);
			saturation = 1 - (blue / green);
		} else {
			hue = (2/6) + (1/6) * (blue - red) / (green - red);
			saturation = 1 - (red / green);
		}
	} else {
		if (green >= red) {
			hue = (4/6) - (1/6) * (green - red) / (blue - red);
			saturation = 1 - (red / green);
		} else {
			hue = (4/6) + (1/6) * (red - green) / (blue - green);
			saturation = 1 - (green / blue);
		}
	}
	result.hue = hue;
	result.saturation = saturation;

	return result;
}

/*
rgb_color
hsl_color::to_rgb() const
{

}*/

hsp_color
hsp_color::set_to(float h, float s, float b)
{
	hue = h;
	saturation = s;
	brightness = b;
	return *this;
}


inline bool
hsp_color::IsDark() const
{
	return brightness <= 0.5f;
}


inline bool
hsp_color::IsLight() const
{
	return brightness > 0.5f;
}


inline float
hsp_color::Contrast(hsp_color colorA, hsp_color colorB)
{
	float contrast = colorA.brightness - colorB.brightness;
	if (contrast < 0)
		return -contrast;

	return contrast;
}
