/*
 *  Copyright 2024, Diego Roux, diegoroux04 at proton dot me
 *  Distributed under the terms of the MIT License.
 */

#include <hmulti_audio.h>
#include <KernelExport.h>
#include <kernel.h>

#include <string.h>

#include "virtio_sound.h"
#include "driver.h"

#define VIRTIO_MULTI_CONTROL_FIRST_ID	1024
#define VIRTIO_MULTI_CONTROL_MASTER_ID	0


static VirtIOSoundPCMInfo*
get_stream(VirtIOSoundDriverInfo* info, uint8 direction)
{
	for (uint32 i = 0; i < info->nStreams; i++) {
		VirtIOSoundPCMInfo* stream = &info->streams[i];

		if (stream->direction == direction)
			return stream;
	}

	return NULL;
}


static void
create_multi_channel_info(VirtIOSoundDriverInfo* info, multi_channel_info* channels)
{
	uint32 index = 0;

	for (uint32 i = 0; i < 2; i++) {
		VirtIOSoundPCMInfo* stream = get_stream(info, i);
		if (stream == NULL)
			continue;

		for (uint8 j = 0; j < stream->channels; j++) {
			channels[index].channel_id = index;

			channels[index].kind = (stream->direction == VIRTIO_SND_D_OUTPUT)
				? B_MULTI_OUTPUT_CHANNEL : B_MULTI_INPUT_CHANNEL;

			channels[index].designations = stream->chmap[j];

			if (stream->channels == 2) {
				channels[index].designations |= B_CHANNEL_STEREO_BUS;
			} else if (stream->channels > 2) {
				channels[index].designations |= B_CHANNEL_SURROUND_BUS;
			}

			channels[index].connectors = 0x00;

			index++;
		}
	}

	for (uint32 i = 0; i < 2; i++) {
		VirtIOSoundPCMInfo* stream = get_stream(info, i);
		if (stream == NULL)
			continue;

		channels[index].channel_id = index;
		channels[index].kind = (stream->direction == VIRTIO_SND_D_OUTPUT)
			? B_MULTI_OUTPUT_BUS : B_MULTI_INPUT_BUS;
		channels[index].designations = B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS;
		channels[index].connectors = B_CHANNEL_MINI_JACK_STEREO;
		index++;

		channels[index].channel_id = index;
		channels[index].kind = (stream->direction == VIRTIO_SND_D_OUTPUT)
			? B_MULTI_OUTPUT_BUS : B_MULTI_INPUT_BUS;
		channels[index].designations = B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS;
		channels[index].connectors = B_CHANNEL_MINI_JACK_STEREO;
		index++;
	}

	return;
}


static status_t
get_description(VirtIOSoundDriverInfo* info, multi_description* desc)
{
	desc->interface_version = B_CURRENT_INTERFACE_VERSION;
	desc->interface_minimum = B_CURRENT_INTERFACE_VERSION;

	strcpy(desc->friendly_name, "Virtio Sound Device");
	strcpy(desc->vendor_info, "Haiku");

	desc->input_channel_count = 0;
	desc->output_channel_count = 0;

	desc->output_bus_channel_count = 0;
	desc->input_bus_channel_count = 0;
	desc->aux_bus_channel_count = 0;

	desc->interface_flags = 0x00;

	for (uint32 i = 0; i < 2; i++) {
		VirtIOSoundPCMInfo* stream = get_stream(info, i);
		if (stream == NULL)
			continue;

		switch (i) {
			case VIRTIO_SND_D_OUTPUT:
				desc->output_channel_count = stream->channels;
				desc->output_bus_channel_count = 2;
				
				desc->output_rates = stream->rates;
				desc->output_formats = stream->formats;
				
				desc->interface_flags |= B_MULTI_INTERFACE_PLAYBACK;
				break;
			case VIRTIO_SND_D_INPUT:
				desc->input_channel_count = stream->channels;
				desc->input_bus_channel_count = 2;
				
				desc->input_rates = stream->rates;
				desc->input_formats = stream->formats;
				
				desc->interface_flags |= B_MULTI_INTERFACE_RECORD;
				break;
		}
	}

	int32 channels = desc->output_channel_count + desc->input_channel_count
		+ desc->output_bus_channel_count + desc->input_bus_channel_count;

	if (desc->request_channel_count >= channels) {
		create_multi_channel_info(info, desc->channels);
	}

	desc->max_cvsr_rate = 384000;
	desc->min_cvsr_rate = 0;

	desc->lock_sources = B_MULTI_LOCK_INTERNAL;
	desc->timecode_sources = 0;

	desc->start_latency = 30000;

	strcpy(desc->control_panel, "");

	return B_OK;
}


static status_t
get_enabled_channels(VirtIOSoundDriverInfo* info, multi_channel_enable* data)
{
	uint32 channels = 0;

	for (uint32 i = 0; i < 2; i++) {
		VirtIOSoundPCMInfo* stream = get_stream(info, i);
		if (stream == NULL)
			continue;

		channels += stream->channels;
	}

	for (uint32 i = 0; i < channels; i++)
		B_SET_CHANNEL(data->enable_bits, i, true);

	data->lock_source = B_MULTI_LOCK_INTERNAL;

	return B_OK;
}


static status_t
get_global_format(VirtIOSoundDriverInfo* info, multi_format_info* data)
{
	memset((void*)data, 0x00, sizeof(multi_format_info));

	data->info_size = sizeof(multi_format_info);

	data->output_latency = 80;
	data->input_latency = 80;

	for (uint32 i = 0; i < 2; i++) {
		VirtIOSoundPCMInfo* stream = get_stream(info, i);
		if (stream == NULL)
			continue;

		_multi_format* reply;
		switch (i) {
			case VIRTIO_SND_D_OUTPUT:
				reply = &data->output;
				break;
			case VIRTIO_SND_D_INPUT:
				reply = &data->input;
				break;
		}

		reply->format = stream->format;
		reply->rate = stream->rate;
		reply->cvsr = stream->cvsr;
	}

	return B_OK;
}


static uint8
format_to_size(uint32 format)
{
	switch (format) {
		case B_FMT_8BIT_S:
		case B_FMT_8BIT_U:
			return 1;
		case B_FMT_16BIT:
			return 2;
		case B_FMT_20BIT:
		case B_FMT_24BIT:
		case B_FMT_32BIT:
		case B_FMT_FLOAT:
			return 4;
		case B_FMT_DOUBLE:
			return 8;
		default:
			return 0;
	}
}


static uint32
to_cvsr(uint32 rate)
{
	switch (rate) {
		case B_SR_8000:
			return 8000;
		case B_SR_11025:
			return 11025;
		case B_SR_16000:
			return 16000;
		case B_SR_22050:
			return 22050;
		case B_SR_32000:
			return 32000;
		case B_SR_44100:
			return 44100;
		case B_SR_48000:
			return 48000;
		case B_SR_64000:
			return 64000;
		case B_SR_88200:
			return 88200;
		case B_SR_96000:
			return 96000;
		case B_SR_176400:
			return 176400;
		case B_SR_192000:
			return 192000;
		case B_SR_384000:
			return 384000;
		default:
			return 0;
	}
}


static const char*
to_format_string(uint32 format)
{
	switch (format) {
		case B_FMT_8BIT_S:
			return "8-bit signed";
		case B_FMT_8BIT_U:
			return "8-bit unsigned";
		case B_FMT_16BIT:
			return "16-bit";
		case B_FMT_20BIT:
			return "20-bit";
		case B_FMT_24BIT:
			return "24-bit";
		case B_FMT_32BIT:
			return "32-bit";
		case B_FMT_FLOAT:
			return "float";
		case B_FMT_DOUBLE:
			return "double";
		default:
			return "unknown";
	}
}


static status_t
set_global_format(VirtIOSoundDriverInfo* info, multi_format_info* data)
{
	for (uint32 i = 0; i < 2; i++) {
		VirtIOSoundPCMInfo* stream = get_stream(info, i);
		if (stream == NULL)
			continue;

		_multi_format* request;
		switch (i) {
			case VIRTIO_SND_D_OUTPUT:
				request = &data->output;
				break;
			case VIRTIO_SND_D_INPUT:
				request = &data->input;
				break;
		}

		if (!(stream->formats & request->format)) {
			ERROR("unsupported format requested (" B_PRIu32 ")\n", request->format);
			return B_BAD_VALUE;
		}

		if (!(stream->rates & request->rate)) {
			ERROR("unsupported rate requested (" B_PRIu32 ")\n", request->rate);
			return B_BAD_VALUE;
		}

		stream->format = request->format;
		stream->rate = request->rate;
		stream->cvsr = to_cvsr(request->rate);

		stream->buffer_size = 1024 * 20;

		stream->period_size = stream->channels * format_to_size(stream->format)
			* stream->buffer_size;

		status_t status;

		if (stream->current_state == VIRTIO_SND_STATE_START) {
			status = VirtIOSoundPCMStop(info, stream);

			if (status != B_OK) {
				ERROR("unable to stop stream [id: " B_PRIu32 "]\n", stream->stream_id);
				return status;
			}
		}

		if (stream->current_state == VIRTIO_SND_STATE_STOP) {
			status = VirtIOSoundPCMRelease(info, stream);

			if (status != B_OK) {
				ERROR("unable to release stream [id: " B_PRIu32 "]\n", stream->stream_id);
				return status;
			}
		}

		status = VirtIOSoundPCMSetParams(info, stream, stream->period_size, stream->period_size);
		if (status != B_OK) {
			ERROR("set params failed (%s)\n", strerror(status));
			return status;
		}

		LOG("%s stream [id: " B_PRIu32 "] set to %s format and " B_PRIu32 " Hz rate\n",
			(stream->direction == VIRTIO_SND_D_OUTPUT) ? "output" : "input",
			stream->stream_id, to_format_string(stream->format), stream->cvsr);
	}

	return B_OK;
}


static status_t
list_mix_channels(VirtIOSoundDriverInfo* info, multi_mix_channel_info* data)
{
	return B_ERROR;
}


static status_t
list_mix_controls(VirtIOSoundDriverInfo* info, multi_mix_control_info* data)
{
	uint8 idx = 0;
	for (uint32 i = 0; i < 2; i++) {
		VirtIOSoundPCMInfo* stream = get_stream(info, i);
		if (stream == NULL)
			continue;

		multi_mix_control* controls = &data->controls[idx];

		controls->id = VIRTIO_MULTI_CONTROL_FIRST_ID + idx;
		controls->parent = 0;
		controls->flags = B_MULTI_MIX_GROUP;
		controls->master = VIRTIO_MULTI_CONTROL_MASTER_ID;
		controls->string = S_null;
		
		switch (i) {
			case VIRTIO_SND_D_OUTPUT:
				strcpy(controls->name, "Playback");
				break;
			case VIRTIO_SND_D_INPUT:
				strcpy(controls->name, "Record");
				break;
		}

		idx++;
	}

	data->control_count = 2;

	return B_OK;
}


static status_t
list_mix_connections(VirtIOSoundDriverInfo* info, multi_mix_connection_info* data)
{
	return B_ERROR;
}


static status_t
get_mix(VirtIOSoundDriverInfo* info, multi_mix_value_info* data)
{
	return B_ERROR;
}


static status_t
set_mix(VirtIOSoundDriverInfo* info, multi_mix_value_info* data)
{
	return B_ERROR;
}


static status_t
get_buffers(VirtIOSoundDriverInfo* info, multi_buffer_list* data)
{
	data->flags = 0x00;

	for (uint32 i = 0; i < 2; i++) {
		VirtIOSoundPCMInfo* stream = get_stream(info, i);
		if (stream == NULL)
			continue;

		buffer_desc** buffers;
		status_t status;
		char* buf_ptr;

		switch (i) {
			case VIRTIO_SND_D_OUTPUT: {
				status = VirtIOSoundTXQueueInit(info, stream);

				data->flags |= B_MULTI_BUFFER_PLAYBACK;

				data->return_playback_buffers = BUFFERS;
				data->return_playback_channels = stream->channels;
				data->return_playback_buffer_size = stream->buffer_size;

				buffers = data->playback_buffers;

				buf_ptr = (char*)info->txBuf;
				break;
			}
			case VIRTIO_SND_D_INPUT: {
				status = VirtIOSoundRXQueueInit(info, stream);

				data->flags |= B_MULTI_BUFFER_RECORD;

				data->return_record_buffers = BUFFERS;
				data->return_record_channels = stream->channels;
				data->return_record_buffer_size = stream->buffer_size;

				buffers = data->record_buffers;

				buf_ptr = (char*)info->rxBuf;
				break;
			}
		}

		if (status != B_OK)
			return status;

		uint32 format_size = format_to_size(stream->format);

		for (uint32 buf_id = 0; buf_id < BUFFERS; buf_id++) {
			if (!IS_USER_ADDRESS(buffers[buf_id]))
				return B_BAD_ADDRESS;

			struct buffer_desc buf_desc[stream->channels];

			for (uint32 ch_id = 0; ch_id < stream->channels; ch_id++) {
				buf_desc[ch_id].base = buf_ptr + (format_size * ch_id);
				buf_desc[ch_id].stride = format_size * stream->channels;
			}

			status = user_memcpy(buffers[buf_id], buf_desc,
				sizeof(struct buffer_desc) * stream->channels);

			if (status < B_OK)
				return B_BAD_ADDRESS;

			buf_ptr += stream->period_size;
		}

		status = VirtIOSoundPCMPrepare(info, stream);
		if (status != B_OK) {
			ERROR("failed to prepare stream_" B_PRIu32 " (%s)\n",
				stream->stream_id, strerror(status));

			return status;
		}
	}

	return B_OK;
}


static status_t
start_stream(VirtIOSoundDriverInfo* info, VirtIOSoundPCMInfo* stream)
{
	stream->buffer_cycle = 0;
	stream->real_time = 0;
	stream->frames_count = 0;

	status_t status;
	if ((void*)stream->xferBuf == NULL) {
		stream->xferArea = create_area("virtio_snd xfer", (void**)&stream->xferBuf,
			B_ANY_KERNEL_BLOCK_ADDRESS, B_PAGE_SIZE, B_FULL_LOCK,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

		status = stream->xferArea;
		if (status < 0) {
			ERROR("unable to create xfer area (%s)\n", strerror(status));
			return status;
		}

		physical_entry entry;
		status = get_memory_map((void*)stream->xferBuf, B_PAGE_SIZE, &entry, 1);
		if (status != B_OK) {
			ERROR("unable to get xfer memory map (%s)\n", strerror(status));
			goto err1;
		}

		stream->xferAddr = entry.address;

		for (uint32 i = 0; i < BUFFERS; i++) {
			struct virtio_snd_pcm_xfer* xfer = (struct virtio_snd_pcm_xfer*)(stream->xferBuf
				+ (sizeof(struct virtio_snd_pcm_xfer) + sizeof(struct virtio_snd_pcm_status)) * i);

			xfer->stream_id = stream->stream_id;
		}
	}

	status = VirtIOSoundPCMStart(info, stream);
	if (status != B_OK) {
		ERROR("unable to start stream_" B_PRIu32 " (%s)\n", stream->stream_id, strerror(status));
		goto err1;
	}

	LOG("stream [id: " B_PRIu32 "] started %s\n", stream->stream_id,
		(stream->direction == VIRTIO_SND_D_OUTPUT) ? "playback" : "recording");

	return B_OK;

err1:
	delete_area(stream->xferArea);
	return status;
}


static status_t
stream_buffer_exchange(VirtIOSoundDriverInfo* info, VirtIOSoundPCMInfo* stream)
{
	snooze_until(stream->real_time + 1000000L / stream->cvsr * stream->buffer_size,
		CLOCK_REALTIME);

	size_t writtenVectorCount = 0;
	size_t readVectorCount = 0;
	::virtio_queue queue = 0;
	addr_t baseAddr = 0;

	switch (stream->direction) {
		case VIRTIO_SND_D_OUTPUT: {
			baseAddr = info->txAddr;
			queue = info->txQueue;

			writtenVectorCount = 1;
			readVectorCount = 2;
			break;
		}
		case VIRTIO_SND_D_INPUT: {
			baseAddr = info->rxAddr;
			queue = info->rxQueue;

			writtenVectorCount = 2;
			readVectorCount = 1;
			break;
		}
	}

	if (!info->virtio->queue_is_empty(queue)) {
		DEBUG("%s", "queue is not empty\n");
		return B_ERROR;
	}

	physical_entry entries[3];

	entries[0].address = stream->xferAddr + (sizeof(struct virtio_snd_pcm_xfer) +
		sizeof(struct virtio_snd_pcm_status)) * stream->buffer_cycle;
	entries[0].size = sizeof(struct virtio_snd_pcm_xfer);

	entries[1].address = baseAddr + stream->period_size * stream->buffer_cycle;
	entries[1].size = stream->period_size;

	entries[2].address = entries[0].address + sizeof(struct virtio_snd_pcm_xfer);
	entries[2].size = sizeof(struct virtio_snd_pcm_status);

	status_t status = info->virtio->queue_request_v(queue, entries,
		readVectorCount, writtenVectorCount, NULL);

	if (status != B_OK) {
		DEBUG("%s", "enqueue request failed\n");
		return status;
	}

	while (!info->virtio->queue_dequeue(queue, NULL, NULL));

	struct virtio_snd_pcm_status* hdr = (struct virtio_snd_pcm_status*)(stream->xferBuf
		+ (sizeof(struct virtio_snd_pcm_xfer) + sizeof(struct virtio_snd_pcm_status))
		* stream->buffer_cycle + sizeof(struct virtio_snd_pcm_xfer));

	if (hdr->status != VIRTIO_SND_S_OK) {
		ERROR("device (stream_" B_PRIu32 ") returned error signal on buffer exchange\n",
			stream->stream_id);

		return B_ERROR;
	}

	stream->real_time = system_time();
	stream->frames_count += stream->buffer_size;
	stream->buffer_cycle = (stream->buffer_cycle + 1) % BUFFERS;

	return B_OK;
}


static status_t
buffer_exchange(VirtIOSoundDriverInfo* info, multi_buffer_info* data)
{
	VirtIOSoundPCMInfo* pStream = get_stream(info, VIRTIO_SND_D_OUTPUT);
	VirtIOSoundPCMInfo* rStream = get_stream(info, VIRTIO_SND_D_INPUT);

	if (pStream != NULL) {
		if (pStream->current_state != VIRTIO_SND_STATE_START)
			start_stream(info, pStream);
	}

	if (rStream != NULL) {
		if (rStream->current_state != VIRTIO_SND_STATE_START)
			start_stream(info, rStream);
	}

	if (!IS_USER_ADDRESS(data))
		return B_BAD_ADDRESS;

	multi_buffer_info buf_info;

	status_t status = user_memcpy(&buf_info, data, sizeof(multi_buffer_info));
	if (status < B_OK)
		return B_BAD_ADDRESS;

	buf_info.flags = 0x00;

	if (pStream != NULL) {
		acquire_sem(info->txSem);

		status = stream_buffer_exchange(info, pStream);
		if (status != B_OK)
			return status;

		buf_info.flags |= B_MULTI_BUFFER_PLAYBACK;

		buf_info.playback_buffer_cycle = pStream->buffer_cycle;
		buf_info.played_real_time = pStream->real_time;
		buf_info.played_frames_count = pStream->frames_count;
	}

	if (rStream != NULL) {
		acquire_sem(info->rxSem);

		status = stream_buffer_exchange(info, rStream);
		if (status != B_OK)
			return status;

		buf_info.flags |= B_MULTI_BUFFER_RECORD;

		buf_info.record_buffer_cycle = rStream->buffer_cycle;
		buf_info.recorded_real_time = rStream->real_time;
		buf_info.recorded_frames_count = rStream->frames_count;
	}

	status = user_memcpy(data, &buf_info, sizeof(multi_buffer_info));
	if (status < B_OK)
		return B_BAD_ADDRESS;

	return B_OK;
}


static status_t
buffer_force_stop(VirtIOSoundDriverInfo* info)
{
	VirtIOSoundPCMInfo* pStream = get_stream(info, VIRTIO_SND_D_OUTPUT);
	VirtIOSoundPCMInfo* rStream = get_stream(info, VIRTIO_SND_D_INPUT);

	if (pStream != NULL) {
		if (pStream->current_state == VIRTIO_SND_STATE_START) {
			acquire_sem(info->txSem);

			status_t status = VirtIOSoundPCMStop(info, pStream);

			if (status != B_OK)
				return status;

			release_sem_etc(info->txSem, 1, B_DO_NOT_RESCHEDULE);
		}
	}

	if (rStream != NULL) {
		if (rStream->current_state == VIRTIO_SND_STATE_START) {
			acquire_sem(info->rxSem);

			status_t status = VirtIOSoundPCMStop(info, rStream);

			if (status != B_OK)
				return status;

			release_sem_etc(info->rxSem, 1, B_DO_NOT_RESCHEDULE);
		}
	}

	delete_area(info->txArea);
	delete_area(info->rxArea);

	info->txBuf = (addr_t)NULL;
	info->rxBuf = (addr_t)NULL;

	delete_sem(info->txSem);
	delete_sem(info->rxSem);

	return B_OK;
}


#define cookie_type VirtIOSoundDriverInfo
#include "../generic/multi.c"

status_t
virtio_snd_ctrl(void* cookie, uint32 op, void* buffer, size_t length)
{
	VirtIOSoundDriverInfo* info = (VirtIOSoundDriverInfo*)cookie;

	return multi_audio_control_generic(info, op, buffer, length);
}