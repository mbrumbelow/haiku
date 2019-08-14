/*
 * Copyright 2019, Ryan Leavengood.
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002, Marcus Overhagen. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


//! A cache for BBuffers to be received by BBufferConsumer::BufferReceived().


#include "BufferCache.h"

#include <Buffer.h>

#include "MediaDebug.h"


namespace BPrivate {


BufferCache::BufferCache()
{
}


BufferCache::~BufferCache()
{
	BufferMap::Iterator iterator = fMap.GetIterator();
	while (iterator.HasNext()) {
		BufferMap::Entry entry = iterator.Next();
		delete entry.value.buffer;
	}
}


BBuffer*
BufferCache::GetBuffer(media_buffer_id id, port_id port)
{
	if (id <= 0)
		return NULL;

	buffer_cache_entry* entry;
	if (fMap.Get(id, entry))
		return entry->buffer;

	buffer_clone_info info;
	info.buffer = id;
	BBuffer* buffer = new(std::nothrow) BBuffer(info);
	if (buffer == NULL || buffer->ID() <= 0
			|| buffer->Data() == NULL) {
		delete buffer;
		return NULL;
	}

	if (buffer->ID() != id)
		debugger("BufferCache::GetBuffer: IDs mismatch");

	try {
		buffer_cache_entry entry;
		entry.buffer = buffer;
		entry.port = port;
		fMap.Put(id, entry);
	} catch (std::bad_alloc& exception) {
		delete buffer;
		return NULL;
	}

	return buffer;
}


void
BufferCache::FlushCacheForPort(port_id port)
{
	BufferMap::Iterator iterator = fMap.GetIterator();
	while (iterator.HasNext()) {
		BufferMap::Entry entry = iterator.Next();
		if (entry.value.port == port) {
			// Delete the buffer
			delete entry.value.buffer;
			// Then remove it from the map
			fMap.Remove(iterator);
		}
	}
}


}	// namespace BPrivate
