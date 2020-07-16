/*
 * Copyright 2010-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 */


#include "AVIFTranslator.h"

#include <BufferIO.h>
#include <Catalog.h>
#include <Messenger.h>
#include <TranslatorRoster.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "avif/avif.h"

#include "ConfigView.h"
#include "TranslatorSettings.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AVIFTranslator"


class FreeAllocation {
	public:
		FreeAllocation(void* buffer)
			:
			fBuffer(buffer)
		{
		}

		~FreeAllocation()
		{
			free(fBuffer);
		}

	private:
		void*	fBuffer;
};



// The input formats that this translator knows how to read
static const translation_format sInputFormats[] = {
	{
		AVIF_IMAGE_FORMAT,
		B_TRANSLATOR_BITMAP,
		AVIF_IN_QUALITY,
		AVIF_IN_CAPABILITY,
		"image/avif",
		"AV1 Image File Format"
	},
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BITS_IN_QUALITY,
		BITS_IN_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (AVIFTranslator)"
	},
};

// The output formats that this translator knows how to write
static const translation_format sOutputFormats[] = {
	{
		AVIF_IMAGE_FORMAT,
		B_TRANSLATOR_BITMAP,
		AVIF_OUT_QUALITY,
		AVIF_OUT_CAPABILITY,
		"image/avif",
		"AV1 Image File Format"
	},
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BITS_OUT_QUALITY,
		BITS_OUT_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (AVIFTranslator)"
	},
};

// Default settings for the Translator
static const TranSetting sDefaultSettings[] = {
	{ B_TRANSLATOR_EXT_HEADER_ONLY, TRAN_SETTING_BOOL, false },
	{ B_TRANSLATOR_EXT_DATA_ONLY, TRAN_SETTING_BOOL, false },
	{ AVIF_SETTING_QUALITY, TRAN_SETTING_INT32, 60 },
	{ AVIF_SETTING_METHOD, TRAN_SETTING_INT32, 2 },
	{ AVIF_SETTING_PREPROCESSING, TRAN_SETTING_BOOL, false },
};

const uint32 kNumInputFormats = sizeof(sInputFormats) /
	sizeof(translation_format);
const uint32 kNumOutputFormats = sizeof(sOutputFormats) /
	sizeof(translation_format);
const uint32 kNumDefaultSettings = sizeof(sDefaultSettings) /
	sizeof(TranSetting);


//	#pragma mark -


AVIFTranslator::AVIFTranslator()
	:
	BaseTranslator(B_TRANSLATE("AVIF images"),
	B_TRANSLATE("AVIF image translator"),
	AVIF_TRANSLATOR_VERSION,
	sInputFormats, kNumInputFormats,
	sOutputFormats, kNumOutputFormats,
	"AVIFTranslator_Settings", sDefaultSettings, kNumDefaultSettings,
	B_TRANSLATOR_BITMAP, AVIF_IMAGE_FORMAT)
{
}


AVIFTranslator::~AVIFTranslator()
{
}


status_t
AVIFTranslator::DerivedIdentify(BPositionIO* stream,
	const translation_format* format, BMessage* settings,
	translator_info* info, uint32 outType)
{
	(void)format;
	(void)settings;
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP)
		return B_NO_TRANSLATOR;

	// Read header and first chunck bytes...
	uint32 buf[64];
	ssize_t size = sizeof(buf);
	if (stream->Read(buf, size) != size)
		return B_IO_ERROR;

	// Check it's a valid AVIF format
	if (memcmp(buf, "\0\0\0 ftypavif", 12) != 0)
		return B_ILLEGAL_DATA;

	info->type = AVIF_IMAGE_FORMAT;
	info->group = B_TRANSLATOR_BITMAP;
	info->quality = AVIF_IN_QUALITY;
	info->capability = AVIF_IN_CAPABILITY;
	snprintf(info->name, sizeof(info->name), B_TRANSLATE("AVIF image"));
	strcpy(info->MIME, "image/avif");

	return B_OK;
}


status_t
AVIFTranslator::DerivedTranslate(BPositionIO* stream,
	const translator_info* info, BMessage* ioExtension, uint32 outType,
	BPositionIO* target, int32 baseType)
{
	(void)info;
	if (baseType == 1)
		// if stream is in bits format
		return _TranslateFromBits(stream, ioExtension, outType, target);
	else if (baseType == 0)
		// if stream is NOT in bits format
		return _TranslateFromAVIF(stream, ioExtension, outType, target);
	else
		// if BaseTranslator dit not properly identify the data as
		// bits or not bits
		return B_NO_TRANSLATOR;
}


BView*
AVIFTranslator::NewConfigView(TranslatorSettings* settings)
{
	return new ConfigView(settings);
}


status_t
AVIFTranslator::_TranslateFromBits(BPositionIO* stream, BMessage* ioExtension,
	uint32 outType, BPositionIO* target)
{
	(void)ioExtension;
	if (!outType)
		outType = AVIF_IMAGE_FORMAT;
	if (outType != AVIF_IMAGE_FORMAT)
		return B_NO_TRANSLATOR;

	TranslatorBitmap bitsHeader;
	status_t status;

	status = identify_bits_header(stream, NULL, &bitsHeader);
	if (status != B_OK)
		return status;

	if (bitsHeader.colors == B_CMAP8) {
		// TODO: support whatever colospace by intermediate colorspace conversion
		printf("Error! Colorspace not supported\n");
		return B_NO_TRANSLATOR;
	}

	int32 bitsBytesPerPixel = 0;
	switch (bitsHeader.colors) {
		case B_RGB32:
		case B_RGB32_BIG:
		case B_RGBA32:
		case B_RGBA32_BIG:
		case B_CMY32:
		case B_CMYA32:
		case B_CMYK32:
			bitsBytesPerPixel = 4;
			break;

		case B_RGB24:
		case B_RGB24_BIG:
		case B_CMY24:
			bitsBytesPerPixel = 3;
			break;

		case B_RGB16:
		case B_RGB16_BIG:
		case B_RGBA15:
		case B_RGBA15_BIG:
		case B_RGB15:
		case B_RGB15_BIG:
			bitsBytesPerPixel = 2;
			break;

		case B_CMAP8:
		case B_GRAY8:
			bitsBytesPerPixel = 1;
			break;

		default:
			return B_ERROR;
	}

	if (bitsBytesPerPixel < 3) {
		// TODO support
		return B_NO_TRANSLATOR;
	}

	//config.quality = (float)fSettings->SetGetInt32(AVIF_SETTING_QUALITY);
	//config.method = fSettings->SetGetInt32(AVIF_SETTING_METHOD);
	//config.preprocessing = fSettings->SetGetBool(AVIF_SETTING_PREPROCESSING);

	int width = bitsHeader.bounds.IntegerWidth() + 1;
	int height = bitsHeader.bounds.IntegerHeight() + 1;
	int depth = 8;
	avifPixelFormat format = AVIF_PIXEL_FORMAT_YUV444;

	avifImage* image = avifImageCreate(width, height, depth, format);
	image->colorPrimaries = AVIF_COLOR_PRIMARIES_BT709;
	image->transferCharacteristics = AVIF_TRANSFER_CHARACTERISTICS_SRGB;
	image->matrixCoefficients = AVIF_MATRIX_COEFFICIENTS_IDENTITY;
	image->yuvRange = AVIF_RANGE_FULL;

	avifRGBImage rgb;
	avifRGBImageSetDefaults(&rgb, image);
	rgb.depth = 8;
	rgb.format = AVIF_RGB_FORMAT_RGB;
	int bitsSize = height * bitsHeader.rowBytes;
	rgb.pixels = static_cast<uint8_t*>(malloc(bitsSize));
	if (rgb.pixels == NULL)
		return B_NO_MEMORY;
	rgb.rowBytes = bitsHeader.rowBytes;

	if (stream->Read(rgb.pixels, bitsSize) != bitsSize) {
		free(rgb.pixels);
		return B_IO_ERROR;
	}

	avifResult conversionResult = avifImageRGBToYUV(image, &rgb);
	free(rgb.pixels);
	if (conversionResult != AVIF_RESULT_OK)
		return B_ERROR;

	avifRWData output = AVIF_DATA_EMPTY;
	avifEncoder* encoder = avifEncoderCreate();
	encoder->maxThreads = 12; // XXX
	encoder->minQuantizer = AVIF_QUANTIZER_LOSSLESS;
	encoder->maxQuantizer = AVIF_QUANTIZER_LOSSLESS;
	avifResult encodeResult = avifEncoderWrite(encoder, image, &output);
	if (encodeResult == AVIF_RESULT_OK) {
	    // output contains a valid .avif file's contents
	    target->Write(output.data, output.size);
	} else {
	    printf("ERROR: Failed to encode: %s\n", avifResultToString(encodeResult));
	}
	avifImageDestroy(image);
	avifRWDataFree(&output);
	avifEncoderDestroy(encoder);

	if (encodeResult != AVIF_RESULT_OK)
		return B_ERROR;
	return B_OK;
}


status_t
AVIFTranslator::_TranslateFromAVIF(BPositionIO* stream, BMessage* ioExtension,
	uint32 outType, BPositionIO* target)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP)
		return B_NO_TRANSLATOR;

	off_t streamLength = 0;
	stream->GetSize(&streamLength);

	off_t streamSize = stream->Seek(0, SEEK_END);
	stream->Seek(0, SEEK_SET);

	void* streamData = malloc(streamSize);
	if (streamData == NULL)
		return B_NO_MEMORY;

	if (stream->Read(streamData, streamSize) != streamSize) {
		free(streamData);
		return B_IO_ERROR;
	}

	avifROData raw;
	raw.data = reinterpret_cast<const uint8_t*>(streamData);
	raw.size = streamSize;

	avifDecoder* decoder = avifDecoderCreate();
	if (decoder == NULL) {
		free(streamData);
		return B_NO_MEMORY;
	}

	avifResult decodeResult = avifDecoderParse(decoder, &raw);
	free(streamData);
	if (decodeResult != AVIF_RESULT_OK)
		// TODO: Return better errors.
		return B_ILLEGAL_DATA;

	// We don’t support animations yet.
	if (decoder->imageCount != 1)
		return B_ILLEGAL_DATA;

	avifResult nextImageResult = avifDecoderNextImage(decoder);
	if (nextImageResult != AVIF_RESULT_OK)
		return B_ILLEGAL_DATA;

	avifImage* image = decoder->image;
	int width = image->width;
	int height = image->height;
	int bpp;
	color_space colors;

	avifRGBImage rgb;
	avifRGBImageSetDefaults(&rgb, image);
	rgb.depth = 8;
	if (image->alphaPlane) {
		rgb.format = AVIF_RGB_FORMAT_BGRA;
		colors = B_RGBA32;
		bpp = 4;
	} else {
		rgb.format = AVIF_RGB_FORMAT_BGR;
		colors = B_RGB24;
		bpp = 3;
	}
	avifRGBImageAllocatePixels(&rgb);

	avifResult conversionResult = avifImageYUVToRGB(image, &rgb);
	if (conversionResult != AVIF_RESULT_OK)
		return B_ILLEGAL_DATA;

	assert((int)rgb.rowBytes == width * bpp);

 	uint32 dataSize = width * bpp * height;

	TranslatorBitmap bitmapHeader;
	bitmapHeader.magic = B_TRANSLATOR_BITMAP;
	bitmapHeader.bounds.Set(0, 0, width - 1, height - 1);
	bitmapHeader.rowBytes = rgb.rowBytes;
	bitmapHeader.colors = colors;
	bitmapHeader.dataSize = dataSize;

	// write out Be's Bitmap header
	swap_data(B_UINT32_TYPE, &bitmapHeader, sizeof(TranslatorBitmap),
		B_SWAP_HOST_TO_BENDIAN);
	ssize_t bytesWritten = target->Write(&bitmapHeader,
		sizeof(TranslatorBitmap));
	if (bytesWritten < B_OK)
		return bytesWritten;

	if ((size_t)bytesWritten != sizeof(TranslatorBitmap))
		return B_IO_ERROR;

	bool headerOnly = false;
	if (ioExtension != NULL)
		ioExtension->FindBool(B_TRANSLATOR_EXT_HEADER_ONLY, &headerOnly);

	if (headerOnly)
		return B_OK;

	uint32 dataLeft = dataSize;
	uint8* p = rgb.pixels;
	while (dataLeft) {
		bytesWritten = target->Write(p, 4);
		if (bytesWritten < B_OK)
			return bytesWritten;

		if (bytesWritten != 4)
			return B_IO_ERROR;

		p += 4;
		dataLeft -= 4;
	}

	return B_OK;
}


#if 0
/* static */ int
AVIFTranslator::_EncodedWriter(const uint8_t* data, size_t dataSize,
	const AVIFPicture* const picture)
{
	BPositionIO* target = (BPositionIO*)picture->custom_ptr;
	return dataSize ? (target->Write(data, dataSize) == (ssize_t)dataSize) : 1;
}
#endif


//	#pragma mark -


BTranslator*
make_nth_translator(int32 n, image_id you, uint32 flags, ...)
{
	(void)you;
	(void)flags;
	if (n != 0)
		return NULL;

	return new AVIFTranslator();
}
