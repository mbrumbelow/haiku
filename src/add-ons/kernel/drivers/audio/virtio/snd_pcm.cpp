/*
 *  Copyright 2024, Diego Roux, diegoroux04 at proton dot me
 *  Distributed under the terms of the MIT License.
 */

#include <virtio.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hmulti_audio.h>

#include "driver.h"
#include "virtio_sound.h"


#define	B_SR_NA		0x00
#define B_FMT_NA	0x00

/* 	High bitrates are currently disabled.
 *	Buffer sizes can't maintain reasonable latency,
 *	resulting in unbearable audio quality.
 *	Until we fix it, they shall remain commented out. */

static const uint32 supportedRates[] = {
	B_SR_NA,		// VIRTIO_SND_PCM_RATE_5512
	B_SR_8000, 		// VIRTIO_SND_PCM_RATE_8000
	B_SR_11025, 	// VIRTIO_SND_PCM_RATE_11025
	B_SR_16000,		// VIRTIO_SND_PCM_RATE_16000
	B_SR_22050,		// VIRTIO_SND_PCM_RATE_22050
	B_SR_32000,		// VIRTIO_SND_PCM_RATE_32000
	B_SR_44100,		// VIRTIO_SND_PCM_RATE_44100
	B_SR_48000,		// VIRTIO_SND_PCM_RATE_48000
	B_SR_64000,		// VIRTIO_SND_PCM_RATE_64000
	B_SR_88200,		// VIRTIO_SND_PCM_RATE_88200
	B_SR_96000,		// VIRTIO_SND_PCM_RATE_96000
	B_SR_NA,		// B_SR_176400,	// VIRTIO_SND_PCM_RATE_176400
	B_SR_NA,		// B_SR_192000,	// VIRTIO_SND_PCM_RATE_192000
	B_SR_NA,		// B_SR_384000,	// VIRTIO_SND_PCM_RATE_384000
};


static const uint32 supportedFormats[] = {
	B_FMT_NA, 		// VIRTIO_SND_PCM_FMT_IMA_ADPCM
	B_FMT_NA,		// VIRTIO_SND_PCM_FMT_MU_LAW
	B_FMT_NA,		// VIRTIO_SND_PCM_FMT_A_LAW
	B_FMT_8BIT_S,	// VIRTIO_SND_PCM_FMT_S8
	B_FMT_8BIT_U,	// VIRTIO_SND_PCM_FMT_U8
	B_FMT_16BIT,	// VIRTIO_SND_PCM_FMT_S16
	B_FMT_NA,		// VIRTIO_SND_PCM_FMT_U16
	B_FMT_NA,		// VIRTIO_SND_PCM_FMT_S18_3
	B_FMT_NA,		// VIRTIO_SND_PCM_FMT_U18_3
	B_FMT_NA,		// VIRTIO_SND_PCM_FMT_S20_3
	B_FMT_NA,		// VIRTIO_SND_PCM_FMT_U20_3
	B_FMT_NA,		// VIRTIO_SND_PCM_FMT_S24_3
	B_FMT_NA,		// VIRTIO_SND_PCM_FMT_U24_3
	B_FMT_20BIT,	// VIRTIO_SND_PCM_FMT_S20
	B_FMT_NA,		// VIRTIO_SND_PCM_FMT_U20
	B_FMT_24BIT,	// VIRTIO_SND_PCM_FMT_S24
	B_FMT_NA,		// VIRTIO_SND_PCM_FMT_U24
	B_FMT_32BIT,	// VIRTIO_SND_PCM_FMT_S32
	B_FMT_NA,		// VIRTIO_SND_PCM_FMT_U32
	B_FMT_FLOAT, 	// VIRTIO_SND_PCM_FMT_FLOAT
	B_FMT_DOUBLE,	// VIRTIO_SND_PCM_FMT_FLOAT64
	B_FMT_NA,		// VIRTIO_SND_PCM_FMT_DSD_U8
	B_FMT_NA,		// VIRTIO_SND_PCM_FMT_DSD_U16
	B_FMT_NA,		// VIRTIO_SND_PCM_FMT_DSD_U32
	B_FMT_NA,		// VIRTIO_SND_PCM_FMT_IEC958_SUBFRAME
};


static uint32
rates_to_multiaudio(struct virtio_snd_pcm_info info)
{
	uint64 rate = (1 << VIRTIO_SND_PCM_RATE_384000);
	uint8 i = VIRTIO_SND_PCM_RATE_384000;

    uint64 rates = B_SR_NA;
	while (rate != 0) {
		if (info.rates & rate)
			rates |= supportedRates[i];

		rate = rate >> 1;
		i--;
	}

	return rates;
}


static uint32
fmts_to_multiaudio(struct virtio_snd_pcm_info info)
{
	uint64 format = (1 << VIRTIO_SND_PCM_FMT_FLOAT64);
	uint8 i = VIRTIO_SND_PCM_FMT_FLOAT64;

    uint64 fmts = B_FMT_NA;
	while (format != 0) {
		if (info.formats & format)
			fmts |= supportedFormats[i];

		format = format >> 1;
		i--;
	}

	return fmts;
}


status_t
VirtIOSoundQueryStreamInfo(VirtIOSoundDriverInfo* info)
{
	struct virtio_snd_pcm_info stream_info[info->nStreams];

	status_t status = VirtIOSoundQueryInfo(info, VIRTIO_SND_R_PCM_INFO, 0,
		info->nStreams, sizeof(struct virtio_snd_pcm_info), (void*)stream_info);

	if (status != B_OK)
		return status;

	info->streams = (VirtIOSoundPCMInfo*)calloc(info->nStreams, sizeof(VirtIOSoundPCMInfo));
	if (info->streams == NULL)
		return B_NO_MEMORY;

	uint32 index = 0;

	for (uint32 i = 0; i < info->nStreams; i++) {
		VirtIOSoundPCMInfo* stream = &info->streams[index++];

		// Dynamic buffer sizes and higher latency settings
		// broke down recording for most bitrates.
		if (stream_info[i].direction == VIRTIO_SND_D_INPUT)
			continue;

		stream->stream_id = i;
		stream->nid = stream_info[i].hdr.hda_fn_nid;

		stream->features = stream_info[i].features;

		stream->formats = fmts_to_multiaudio(stream_info[i]);
		if (!stream->formats) {
			ERROR("stream_%" B_PRIu32 ": unsupported PCM formats (%" B_PRIu32 ")\n",
				i, stream->formats);

			status = B_ERROR;
			goto err1;
		}

		stream->rates = rates_to_multiaudio(stream_info[i]);
		if (!stream->rates) {
			ERROR("stream_%" B_PRIu32 ": unsupported PCM rates (%" B_PRIu32 ")\n",
				i, stream->formats);

			status = B_ERROR;
			goto err1;
		}

		switch (stream_info[i].direction) {
			case VIRTIO_SND_D_OUTPUT:
				info->outputStreams++;
				break;
			case VIRTIO_SND_D_INPUT:
				info->inputStreams++;
				break;
			default:
				ERROR("unknown stream direction (%" B_PRIu32 ")\n", stream_info[i].direction);
				status = B_ERROR;
				goto err1;
		}

		stream->direction = stream_info[i].direction;

		if (stream_info[i].channels_min > stream_info[i].channels_max) {
			ERROR("invalid channel range (%" B_PRIu32 "-%" B_PRIu32 ")\n",
				stream_info[i].channels_min, stream_info[i].channels_max);

			status = B_ERROR;
			goto err1;
		}

		stream->channels_min = stream_info[i].channels_min;
		stream->channels_max = stream_info[i].channels_max;
	}

	return B_OK;

err1:
	free(info->streams);
	return status;
}


static uint8
to_virtio_fmt(uint32 format)
{
	switch (format) {
		case B_FMT_8BIT_S:
			return VIRTIO_SND_PCM_FMT_S8;
		case B_FMT_8BIT_U:
			return VIRTIO_SND_PCM_FMT_U8;
		case B_FMT_16BIT:
			return VIRTIO_SND_PCM_FMT_S16;
		case B_FMT_20BIT:
			return VIRTIO_SND_PCM_FMT_S20;
		case B_FMT_24BIT:
			return VIRTIO_SND_PCM_FMT_S24;
		case B_FMT_32BIT:
			return VIRTIO_SND_PCM_FMT_S32;
		case B_FMT_FLOAT:
			return VIRTIO_SND_PCM_FMT_FLOAT;
		case B_FMT_DOUBLE:
			return VIRTIO_SND_PCM_FMT_FLOAT64;
		default:
			return B_FMT_NA;
	}
}


static uint8
to_virtio_rate(uint32 rate)
{
	switch (rate) {
		case B_SR_8000:
			return VIRTIO_SND_PCM_RATE_8000;
		case B_SR_11025:
			return VIRTIO_SND_PCM_RATE_11025;
		case B_SR_16000:
			return VIRTIO_SND_PCM_RATE_16000;
		case B_SR_22050:
			return VIRTIO_SND_PCM_RATE_22050;
		case B_SR_32000:
			return VIRTIO_SND_PCM_RATE_32000;
		case B_SR_44100:
			return VIRTIO_SND_PCM_RATE_44100;
		case B_SR_48000:
			return VIRTIO_SND_PCM_RATE_48000;
		case B_SR_64000:
			return VIRTIO_SND_PCM_RATE_64000;
		case B_SR_88200:
			return VIRTIO_SND_PCM_RATE_88200;
		case B_SR_96000:
			return VIRTIO_SND_PCM_RATE_96000;
		case B_SR_176400:
			return VIRTIO_SND_PCM_RATE_176400;
		case B_SR_192000:
			return VIRTIO_SND_PCM_RATE_192000;
		case B_SR_384000:
			return VIRTIO_SND_PCM_RATE_384000;
		default:
			return B_SR_NA;
	}
}


status_t
VirtIOSoundPCMSetParams(VirtIOSoundDriverInfo* info, VirtIOSoundPCMInfo* stream,
	uint32 buffer, uint32 period)
{
	if ((stream->current_state != VIRTIO_SND_STATE_SET_PARAMETERS) &&
		(stream->current_state != VIRTIO_SND_STATE_PREPARE) &&
		(stream->current_state != VIRTIO_SND_STATE_RELEASE))
		return B_NOT_ALLOWED;

	struct virtio_snd_pcm_set_params data;

	data.hdr.hdr.code = VIRTIO_SND_R_PCM_SET_PARAMS;
	data.hdr.stream_id = stream->stream_id;

	data.buffer_bytes = buffer;
	data.period_bytes = period;

	data.features = 0;

	data.channels = stream->channels;
	data.format = to_virtio_fmt(stream->format);
	data.rate = to_virtio_rate(stream->rate);

	data.padding = 0;

	status_t status = VirtIOSoundPCMControlRequest(info, (void*)&data,
		sizeof(struct virtio_snd_pcm_set_params));

	if (status != B_OK) {
		ERROR("PCM control request failed.\n");
		return status;
	}

	stream->current_state = VIRTIO_SND_STATE_SET_PARAMETERS;

	return B_OK;
}


status_t
VirtIOSoundPCMPrepare(VirtIOSoundDriverInfo* info, VirtIOSoundPCMInfo* stream)
{
	if ((stream->current_state != VIRTIO_SND_STATE_SET_PARAMETERS) &&
		(stream->current_state != VIRTIO_SND_STATE_PREPARE) &&
		(stream->current_state != VIRTIO_SND_STATE_RELEASE))
		return B_NOT_ALLOWED;

	struct virtio_snd_pcm_prepare data;

	data.hdr.hdr.code = VIRTIO_SND_R_PCM_PREPARE;
	data.hdr.stream_id = stream->stream_id;

	status_t status = VirtIOSoundPCMControlRequest(info, (void*)&data,
		sizeof(struct virtio_snd_pcm_prepare));

	if (status != B_OK)
		return status;

	stream->current_state = VIRTIO_SND_STATE_PREPARE;

	return B_OK;
}


status_t
VirtIOSoundPCMStart(VirtIOSoundDriverInfo* info, VirtIOSoundPCMInfo* stream)
{
	if ((stream->current_state != VIRTIO_SND_STATE_PREPARE) &&
		(stream->current_state != VIRTIO_SND_STATE_STOP))
		return B_NOT_ALLOWED;

	struct virtio_snd_pcm_start data;

	data.hdr.hdr.code = VIRTIO_SND_R_PCM_START;
	data.hdr.stream_id = stream->stream_id;

	status_t status = VirtIOSoundPCMControlRequest(info, (void*)&data,
		sizeof(struct virtio_snd_pcm_prepare));

	if (status != B_OK)
		return status;

	stream->current_state = VIRTIO_SND_STATE_START;

	return B_OK;
}


status_t
VirtIOSoundPCMStop(VirtIOSoundDriverInfo* info, VirtIOSoundPCMInfo* stream)
{
	if (stream->current_state != VIRTIO_SND_STATE_START)
		return B_NOT_ALLOWED;

	struct virtio_snd_pcm_start data;

	data.hdr.hdr.code = VIRTIO_SND_R_PCM_STOP;
	data.hdr.stream_id = stream->stream_id;

	status_t status = VirtIOSoundPCMControlRequest(info, (void*)&data,
		sizeof(struct virtio_snd_pcm_prepare));

	if (status != B_OK)
		return status;

	stream->current_state = VIRTIO_SND_STATE_STOP;

	return B_OK;
}


status_t
VirtIOSoundPCMRelease(VirtIOSoundDriverInfo* info, VirtIOSoundPCMInfo* stream)
{
	if ((stream->current_state != VIRTIO_SND_STATE_PREPARE) &&
		(stream->current_state != VIRTIO_SND_STATE_STOP))
		return B_NOT_ALLOWED;

	struct virtio_snd_pcm_start data;

	data.hdr.hdr.code = VIRTIO_SND_R_PCM_RELEASE;
	data.hdr.stream_id = stream->stream_id;

	status_t status = VirtIOSoundPCMControlRequest(info, (void*)&data,
		sizeof(struct virtio_snd_pcm_prepare));

	if (status != B_OK)
		return status;

	stream->current_state = VIRTIO_SND_STATE_RELEASE;

	return B_OK;
}