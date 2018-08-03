/*
 * Copyright (C) 2018 Haiku, inc.
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef AVCODEC_BASE_H
#define AVCODEC_BASE_H


#include <MediaFormats.h>

extern "C" {
	#include "avcodec.h"
}

class AVCodecBase {
public:
			AVCodecBase();
	virtual		~AVCodecBase();
protected:

			AVCodec*		fCodec;
			AVCodecContext*		fContext;

			enum {
				CODEC_INIT_NEEDED = 0,
				CODEC_INIT_DONE,
				CODEC_INIT_FAILED
			};
			uint32			fCodecInitStatus;
};

#endif // AVCODEC_BASE_H
