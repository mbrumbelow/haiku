/*
 * Copyright 2004-2007, Marcus Overhagen. All rights reserved.
 * Copyright 2008, Maurice Kalinowski. All rights reserved.
 * Copyright 2009-2012, Axel Dörfler, axeld@pinc-software.de.
 *
 * Distributed under the terms of the MIT License.
 */


#include "MediaExtractor.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Autolock.h>

#include "ChunkCache.h"
#include "MediaDebug.h"
#include "MediaMisc.h"
#include "PluginManager.h"


// should be 0, to disable the chunk cache set it to 1
#define DISABLE_CHUNK_CACHE 0


static inline int32
get_bytes_per_row(color_space colorSpace, int32 width)
{
	int32 bpr = 0;
	switch (colorSpace) {
		// supported
		case B_RGB32: case B_RGBA32:
		case B_RGB32_BIG: case B_RGBA32_BIG:
		case B_UVL32: case B_UVLA32:
		case B_LAB32: case B_LABA32:
		case B_HSI32: case B_HSIA32:
		case B_HSV32: case B_HSVA32:
		case B_HLS32: case B_HLSA32:
		case B_CMY32: case B_CMYA32: case B_CMYK32:
			bpr = 4 * width;
			break;
		case B_RGB24: case B_RGB24_BIG:
		case B_UVL24: case B_LAB24: case B_HSI24:
		case B_HSV24: case B_HLS24: case B_CMY24:
			bpr = 3 * width;
			break;
		case B_RGB16:	   case B_RGB15:	   case B_RGBA15:
		case B_RGB16_BIG:   case B_RGB15_BIG:   case B_RGBA15_BIG:
			bpr = 2 * width;
			break;
		case B_CMAP8: case B_GRAY8:
			bpr = width;
			break;
		case B_GRAY1:
			bpr = (width + 7) / 8;
			break;
		case B_YCbCr422: case B_YUV422:
			bpr = (width + 3) / 4 * 8;
			break;
		case B_YCbCr411: case B_YUV411:
			bpr = (width + 3) / 4 * 6;
			break;
		case B_YCbCr444: case B_YUV444:
			bpr = (width + 3) / 4 * 12;
			break;
		case B_YCbCr420: case B_YUV420:
			bpr = (width + 3) / 4 * 6;
			break;
		case B_YUV9:
			bpr = (width + 15) / 16 * 18;
			break;
		// unsupported
		case B_NO_COLOR_SPACE:
		case B_YUV12:
			break;
	}
	return bpr;
}


class MediaExtractorChunkProvider : public ChunkProvider {
public:
	MediaExtractorChunkProvider(MediaExtractor* extractor, int32 stream)
		:
		fExtractor(extractor),
		fStream(stream)
	{
	}

	virtual status_t GetNextChunk(const void** _chunkBuffer, size_t* _chunkSize,
		media_header *mediaHeader)
	{
		return fExtractor->GetNextChunk(fStream, _chunkBuffer, _chunkSize,
			mediaHeader);
	}

private:
	MediaExtractor*	fExtractor;
	int32			fStream;
};


// #pragma mark -


MediaExtractor::MediaExtractor(BDataIO* source, int32 flags)
	:
	fExtractorThread(-1),
	fReader(NULL),
	fStreamInfo(NULL),
	fStreamCount(0)
{
	_Init(source, flags);
}


void
MediaExtractor::_Init(BDataIO* source, int32 flags)
{
	CALLED();

	fSource = source;

#if !DISABLE_CHUNK_CACHE
	// start extractor thread
	fExtractorWaitSem = create_sem(1, "media extractor thread sem");
	if (fExtractorWaitSem < 0) {
		fInitStatus = fExtractorWaitSem;
		return;
	}
#endif

	fInitStatus = gPluginManager.CreateReader(&fReader, &fStreamCount,
		&fFileFormat, source);
	if (fInitStatus != B_OK)
		return;

	fStreamInfo = new stream_info[fStreamCount];

	// initialize stream infos
	for (int32 i = 0; i < fStreamCount; i++) {
		fStreamInfo[i].status = B_OK;
		fStreamInfo[i].cookie = 0;
		fStreamInfo[i].hasCookie = false;
		fStreamInfo[i].infoBuffer = 0;
		fStreamInfo[i].infoBufferSize = 0;
		fStreamInfo[i].lastChunk = NULL;
		fStreamInfo[i].chunkCache = NULL;
		fStreamInfo[i].encodedFormat.Clear();
	}

	// create all stream cookies
	for (int32 i = 0; i < fStreamCount; i++) {
		if (fReader->AllocateCookie(i, &fStreamInfo[i].cookie) != B_OK) {
			fStreamInfo[i].cookie = 0;
			fStreamInfo[i].hasCookie = false;
			fStreamInfo[i].status = B_ERROR;
			ERROR("MediaExtractor::MediaExtractor: AllocateCookie for stream %"
				B_PRId32 " failed\n", i);
		} else
			fStreamInfo[i].hasCookie = true;
	}

	// get info for all streams
	for (int32 i = 0; i < fStreamCount; i++) {
		if (fStreamInfo[i].status != B_OK)
			continue;

		int64 frameCount;
		bigtime_t duration;
		if (fReader->GetStreamInfo(fStreamInfo[i].cookie, &frameCount,
				&duration, &fStreamInfo[i].encodedFormat,
				&fStreamInfo[i].infoBuffer, &fStreamInfo[i].infoBufferSize)
					!= B_OK) {
			fStreamInfo[i].status = B_ERROR;
			ERROR("MediaExtractor::MediaExtractor: GetStreamInfo for "
				"stream %" B_PRId32 " failed\n", i);
		}

		// Allocate our ChunkCache
		size_t chunkCacheMaxBytes = _CalculateChunkBuffer(i);
		fStreamInfo[i].chunkCache
			= new ChunkCache(fExtractorWaitSem, chunkCacheMaxBytes);
		if (fStreamInfo[i].chunkCache->InitCheck() != B_OK) {
			fInitStatus = B_NO_MEMORY;
			return;
		}
	}

#if !DISABLE_CHUNK_CACHE
	// start extractor thread
	fExtractorThread = spawn_thread(_ExtractorEntry, "media extractor thread",
		B_NORMAL_PRIORITY + 4, this);
	resume_thread(fExtractorThread);
#endif
}


MediaExtractor::~MediaExtractor()
{
	CALLED();

	// stop the extractor thread, if still running
	StopProcessing();

	// free all stream cookies
	// and chunk caches
	for (int32 i = 0; i < fStreamCount; i++) {
		if (fStreamInfo[i].hasCookie)
			fReader->FreeCookie(fStreamInfo[i].cookie);

		delete fStreamInfo[i].chunkCache;
	}

	gPluginManager.DestroyReader(fReader);

	delete[] fStreamInfo;
	// fSource is owned by the BMediaFile
}


status_t
MediaExtractor::InitCheck()
{
	CALLED();
	return fInitStatus;
}


void
MediaExtractor::GetFileFormatInfo(media_file_format* fileFormat) const
{
	CALLED();
	*fileFormat = fFileFormat;
}


status_t
MediaExtractor::GetMetaData(BMessage* _data) const
{
	CALLED();
	return fReader->GetMetaData(_data);
}


int32
MediaExtractor::StreamCount()
{
	CALLED();
	return fStreamCount;
}


const char*
MediaExtractor::Copyright()
{
	return fReader->Copyright();
}


const media_format*
MediaExtractor::EncodedFormat(int32 stream)
{
	return &fStreamInfo[stream].encodedFormat;
}


int64
MediaExtractor::CountFrames(int32 stream) const
{
	CALLED();
	if (fStreamInfo[stream].status != B_OK)
		return 0LL;

	int64 frameCount;
	bigtime_t duration;
	media_format format;
	const void* infoBuffer;
	size_t infoSize;

	fReader->GetStreamInfo(fStreamInfo[stream].cookie, &frameCount, &duration,
		&format, &infoBuffer, &infoSize);

	return frameCount;
}


bigtime_t
MediaExtractor::Duration(int32 stream) const
{
	CALLED();

	if (fStreamInfo[stream].status != B_OK)
		return 0LL;

	int64 frameCount;
	bigtime_t duration;
	media_format format;
	const void* infoBuffer;
	size_t infoSize;

	fReader->GetStreamInfo(fStreamInfo[stream].cookie, &frameCount, &duration,
		&format, &infoBuffer, &infoSize);

	return duration;
}


status_t
MediaExtractor::Seek(int32 stream, uint32 seekTo, int64* _frame,
	bigtime_t* _time)
{
	CALLED();

	stream_info& info = fStreamInfo[stream];
	if (info.status != B_OK)
		return info.status;

	BAutolock _(info.chunkCache);

	status_t status = fReader->Seek(info.cookie, seekTo, _frame, _time);
	if (status != B_OK)
		return status;

	// clear buffered chunks after seek
	info.chunkCache->MakeEmpty();

	return B_OK;
}


status_t
MediaExtractor::FindKeyFrame(int32 stream, uint32 seekTo, int64* _frame,
	bigtime_t* _time) const
{
	CALLED();

	stream_info& info = fStreamInfo[stream];
	if (info.status != B_OK)
		return info.status;

	return fReader->FindKeyFrame(info.cookie, seekTo, _frame, _time);
}


status_t
MediaExtractor::GetNextChunk(int32 stream, const void** _chunkBuffer,
	size_t* _chunkSize, media_header* mediaHeader)
{
	stream_info& info = fStreamInfo[stream];

	if (info.status != B_OK)
		return info.status;

#if DISABLE_CHUNK_CACHE
	return fReader->GetNextChunk(fStreamInfo[stream].cookie, _chunkBuffer,
		_chunkSize, mediaHeader);
#else
	BAutolock _(info.chunkCache);

	_RecycleLastChunk(info);

	// Retrieve next chunk - read it directly, if the cache is drained
	chunk_buffer* chunk = info.chunkCache->NextChunk(fReader, info.cookie);

	if (chunk == NULL)
		return B_NO_MEMORY;

	info.lastChunk = chunk;

	*_chunkBuffer = chunk->buffer;
	*_chunkSize = chunk->size;
	*mediaHeader = chunk->header;

	return chunk->status;
#endif
}


status_t
MediaExtractor::CreateDecoder(int32 stream, Decoder** _decoder,
	media_codec_info* codecInfo)
{
	CALLED();

	status_t status = fStreamInfo[stream].status;
	if (status != B_OK) {
		ERROR("MediaExtractor::CreateDecoder can't create decoder for "
			"stream %" B_PRId32 ": %s\n", stream, strerror(status));
		return status;
	}

	// TODO: Here we should work out a way so that if there is a setup
	// failure we can try the next decoder
	Decoder* decoder;
	status = gPluginManager.CreateDecoder(&decoder,
		fStreamInfo[stream].encodedFormat);
	if (status != B_OK) {
#if DEBUG
		char formatString[256];
		string_for_format(fStreamInfo[stream].encodedFormat, formatString,
			sizeof(formatString));

		ERROR("MediaExtractor::CreateDecoder gPluginManager.CreateDecoder "
			"failed for stream %" B_PRId32 ", format: %s: %s\n", stream,
			formatString, strerror(status));
#endif
		return status;
	}

	ChunkProvider* chunkProvider
		= new(std::nothrow) MediaExtractorChunkProvider(this, stream);
	if (chunkProvider == NULL) {
		gPluginManager.DestroyDecoder(decoder);
		ERROR("MediaExtractor::CreateDecoder can't create chunk provider "
			"for stream %" B_PRId32 "\n", stream);
		return B_NO_MEMORY;
	}

	decoder->SetChunkProvider(chunkProvider);

	status = decoder->Setup(&fStreamInfo[stream].encodedFormat,
		fStreamInfo[stream].infoBuffer, fStreamInfo[stream].infoBufferSize);
	if (status != B_OK) {
		gPluginManager.DestroyDecoder(decoder);
		ERROR("MediaExtractor::CreateDecoder Setup failed for stream %" B_PRId32
			": %s\n", stream, strerror(status));
		return status;
	}

	status = gPluginManager.GetDecoderInfo(decoder, codecInfo);
	if (status != B_OK) {
		gPluginManager.DestroyDecoder(decoder);
		ERROR("MediaExtractor::CreateDecoder GetCodecInfo failed for stream %"
			B_PRId32 ": %s\n", stream, strerror(status));
		return status;
	}

	*_decoder = decoder;
	return B_OK;
}


status_t
MediaExtractor::GetStreamMetaData(int32 stream, BMessage* _data) const
{
	const stream_info& info = fStreamInfo[stream];

	if (info.status != B_OK)
		return info.status;

	return fReader->GetStreamMetaData(fStreamInfo[stream].cookie, _data);
}


void
MediaExtractor::StopProcessing()
{
#if !DISABLE_CHUNK_CACHE
	if (fExtractorWaitSem > -1) {
		// terminate extractor thread
		delete_sem(fExtractorWaitSem);
		fExtractorWaitSem = -1;

		status_t status;
		wait_for_thread(fExtractorThread, &status);
	}
#endif
}


void
MediaExtractor::_RecycleLastChunk(stream_info& info)
{
	if (info.lastChunk != NULL) {
		info.chunkCache->RecycleChunk(info.lastChunk);
		info.lastChunk = NULL;
	}
}


status_t
MediaExtractor::_ExtractorEntry(void* extractor)
{
	static_cast<MediaExtractor*>(extractor)->_ExtractorThread();
	return B_OK;
}


size_t
MediaExtractor::_CalculateChunkBuffer(int32 stream)
{
	const media_format* format = EncodedFormat(stream);
	size_t cacheSize = 3 * 1024 * 1024;
	int32 rowSize = get_bytes_per_row(format->ColorSpace(), format->Width());
	if (rowSize > 0) {
		// Frame size, multipled by 2 for good measure
		cacheSize = (rowSize * format->Height()) * 2;
	}
	return ROUND_UP_TO_PAGE(cacheSize);
}


void
MediaExtractor::_ExtractorThread()
{
	while (true) {
		status_t status;
		do {
			status = acquire_sem(fExtractorWaitSem);
		} while (status == B_INTERRUPTED);

		if (status != B_OK) {
			// we were asked to quit
			return;
		}

		// Iterate over all streams until they are all filled

		int32 streamsFilled;
		do {
			streamsFilled = 0;

			for (int32 stream = 0; stream < fStreamCount; stream++) {
				stream_info& info = fStreamInfo[stream];
				if (info.status != B_OK) {
					streamsFilled++;
					continue;
				}

				BAutolock _(info.chunkCache);

				if (!info.chunkCache->SpaceLeft()
					|| !info.chunkCache->ReadNextChunk(fReader, info.cookie))
					streamsFilled++;
			}
		} while (streamsFilled < fStreamCount);
	}
}
