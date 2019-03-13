/*
 * Copyright 2018, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <CodecRoster.h>

#include <MediaFormats.h>

#include "PluginManager.h"


status_t
BCodecRoster::InstantiateReader(Reader** reader, int32* streamCount,
	media_file_format* mff, BDataIO* source)
{
	return gPluginManager.CreateReader(reader, streamCount, mff, source);
}


void
BCodecRoster::ReleaseReader(Reader* reader)
{
	return gPluginManager.DestroyReader(reader);
}


status_t
BCodecRoster::InstantiateDecoder(Decoder** decoder, const media_format& format)
{
	return gPluginManager.CreateDecoder(decoder, format);
}


status_t
BCodecRoster::InstantiateDecoder(Decoder** decoder, const media_codec_info& mci)
{
	return gPluginManager.CreateDecoder(decoder, mci);
}


void
BCodecRoster::ReleaseDecoder(Decoder* decoder)
{
	return gPluginManager.DestroyDecoder(decoder);
}


status_t
BCodecRoster::InstantiateWriter(Writer** writer, const media_file_format& mff,
	BDataIO* target)
{
	return gPluginManager.CreateWriter(writer, mff, target);
}


void
BCodecRoster::ReleaseWriter(Writer* writer)
{
	return gPluginManager.DestroyWriter(writer);
}


status_t
BCodecRoster::InstantiateEncoder(Encoder** encoder, const media_format& format)
{
	return gPluginManager.CreateEncoder(encoder, format);
}


status_t
BCodecRoster::InstantiateEncoder(Encoder** encoder,
	const media_codec_info* codecInfo, uint32 flags)
{
	return gPluginManager.CreateEncoder(encoder, codecInfo, flags);
}


void
BCodecRoster::ReleaseEncoder(Encoder* encoder)
{
	return gPluginManager.DestroyEncoder(encoder);
}


status_t
BCodecRoster::InstantiateStreamer(Streamer** streamer, BUrl url,
	BDataIO** source)
{
	return gPluginManager.CreateStreamer(streamer, url, source);
}


void
BCodecRoster::ReleaseStreamer(Streamer* streamer)
{
	return gPluginManager.DestroyStreamer(streamer);
}


status_t
BCodecRoster::GetNextEncoder(int32* cookie, const media_file_format* fileFormat,
	const media_format* inputFormat, media_format* _outputFormat,
	media_codec_info* _codecInfo)
{
	return get_next_encoder(cookie, fileFormat,
		inputFormat, _outputFormat, _codecInfo);
}


status_t
BCodecRoster::GetNextEncoder(int32* cookie, const media_file_format* fileFormat,
	const media_format* inputFormat, const media_format* outputFormat,
	media_codec_info* _codecInfo, media_format* _acceptedInputFormat,
	media_format* _acceptedOutputFormat)
{
	return get_next_encoder(cookie, fileFormat, inputFormat, outputFormat,
		_codecInfo, _acceptedInputFormat, _acceptedOutputFormat);
}


status_t
BCodecRoster::GetNextEncoder(int32* cookie, media_codec_info* _codecInfo)
{
	return get_next_encoder(cookie, _codecInfo);
}
