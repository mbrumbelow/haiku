/*
 * Copyright 2025 Pascal Abresch. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <HSP.h>

#include <math.h>

hsp_color
hsp_color::from_rgb(const rgb_color& rgbcolor)
{
	return from_float(rgbcolor.red / 255.0f, rgbcolor.green / 255.0f, rgbcolor.blue / 255.0f);
}


hsp_color
hsp_color::from_float(float red, float green, float blue)
{
	hsp_color result;

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
			hue = (6.0f/6.0f) - ((1.0f/6.0f) * (blue - green) / (red - green));
			saturation = 1 - (green / red);
		} else {
			hue = (0.0f/6.0f) + ((1.0f/6.0f) * (green - blue) / (green - blue));
			saturation = 1 - (green / red);
		}
	} else if (green >= red && green <= blue) {
		if (red >= blue) {
			hue = (2.0f/6.0f) - ((1.0f/6.0f) * (red - blue) / (green - blue));
			saturation = 1 - (blue / green);
		} else {
			hue = (2.0f/6.0f) + ((1.0f/6.0f) * (blue - red) / (green - red));
			saturation = 1 - (red / green);
		}
	} else {
		if (green >= red) {
			hue = (4.0f/6.0f) - ((1.0f/6.0f) * (green - red) / (blue - red));
			saturation = 1 - (red / green);
		} else {
			hue = (4.0f/6.0f) + ((1.0f/6.0f) * (red - green) / (blue - green));
			saturation = 1 - (green / blue);
		}
	}
	result.hue = hue;
	result.saturation = saturation;

	return result;
}


rgb_color
hsp_color::to_rgb() const
{
	float invertedSaturation = 1 - saturation;

	float tempHue;

	rgb_color result;

	float redPart = .299;
	float greenPart =.587;
	float bluePart = .114;

	// Take the short path when the saturation is exactly 1
	if (saturation == 1.0f) {
		float brightnessSquare = brightness * brightness;

		if (hue < 1.0f / 6.0f) {
			tempHue = 6.0f * (hue - (0.f/6.0f));
			result.red   = sqrt( brightnessSquare / (redPart + (greenPart * tempHue * tempHue)));
			result.green = result.red * tempHue;
			result.blue  = 0.0;
		} else if (hue < 2.0f / 6.0f) {
			tempHue = 6.0f * (hue + (2.f/6.0f));
			result.green = sqrt( brightnessSquare / (greenPart + (redPart * tempHue * tempHue)));
			result.red   = result.green * tempHue;
			result.blue  = 0.0f;
		} else if (hue < 3.0f / 6.0f) {
			tempHue = 6.0f * (hue - (2.f/6.0f));
			result.green = sqrt( brightnessSquare / (greenPart + (bluePart * tempHue * tempHue)));
			result.blue  = result.green * tempHue;
			result.red   = 0.0f;
		} else if (hue < 4.0f / 6.0f) {
			tempHue = 6.0f * (hue + (4.0f/6.0f));
			result.blue  = sqrt( brightnessSquare / (bluePart + (greenPart * tempHue * tempHue)));
			result.green = result.blue * tempHue;
			result.red   = 0.0f;
		} else if (hue < 5.0f / 6.0f) {
			tempHue = 6.0f * (hue - (4.0f/6.0f));
			result.blue  = sqrt( brightnessSquare / (bluePart + (redPart * tempHue * tempHue)));
			result.red   = result.blue * tempHue;
			result.green = 0.0f;
		} else /* if (hue < 6.0f / 6.0f) */{
			tempHue = 6.0f * (hue + (6.f/6.0f));
			result.red   = sqrt( brightnessSquare / (redPart + (bluePart * tempHue * tempHue)));
			result.blue  = result.red * tempHue;
			result.green = 0;
		}
		result.red = roundf(result.red * 255);
		result.green = roundf(result.green * 255);
		result.blue = roundf(result.blue * 255);
		return result;
	}

	float tempPart = 1 - saturation;
	tempPart = 1.0f + (hue * ((1.0f / invertedSaturation) - 1.0f));
	tempPart = tempPart * tempPart;

	if (hue < 1.0f / 6.0f) {
		tempHue = 6.0f * (hue - (0.f/6.0f));
		result.blue = brightness / sqrt((redPart / invertedSaturation / invertedSaturation)
			+ (greenPart * tempPart) + bluePart);
		result.red = result.blue / invertedSaturation;
		result.green = result.blue + (hue * (result.red - result.blue));
	} else if (hue < 2.0f / 6.0f) {
		tempHue = 6.0f * (hue + (2.f/6.0f));
		result.blue = brightness / sqrt((greenPart / invertedSaturation / invertedSaturation)
			+ (redPart * tempPart) + bluePart);
		result.green = result.blue / invertedSaturation;
		result.red = result.blue + (hue * (result.green - result.blue));
	} else if (hue < 3.0f / 6.0f) {
		tempHue = 6.0f * (hue - (2.f/6.0f));
		result.red = brightness / sqrt((greenPart / invertedSaturation / invertedSaturation)
			+ (bluePart * tempPart) + redPart);
		result.green= result.red / invertedSaturation;
		result.blue = result.red + (hue * (result.green - result.red));
	} else if (hue < 4.0f / 6.0f) {
		tempHue = 6.0f * (hue + (4.0f/6.0f));
		result.red = brightness / sqrt((bluePart / invertedSaturation / invertedSaturation)
			+ (greenPart * tempPart) + redPart);
		result.blue = result.red / invertedSaturation;
		result.green = result.red + (hue * (result.blue - result.red));
	} else if (hue < 5.0f / 6.0f) {
		tempHue = 6.0f * (hue - (4.0f/6.0f));
		result.green = brightness / sqrt((bluePart / invertedSaturation / invertedSaturation)
			+ (redPart * tempPart) + greenPart);
		result.blue = result.green / invertedSaturation;
		result.red = result.green + (hue * (result.blue - result.green));
	} else /* if (hue < 6.0f / 6.0f) */{
		tempHue = 6.0f * (hue + (6.f/6.0f));
		result.green = brightness / sqrt((redPart / invertedSaturation / invertedSaturation)
			+ (bluePart * tempPart) + greenPart);
		result.red = result.green / invertedSaturation;
		result.blue = result.green + (hue * (result.red - result.green));
	}
	result.red = roundf(result.red * 255);
	result.green = roundf(result.green * 255);
	result.blue = roundf(result.blue * 255);
	return result;
}

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
