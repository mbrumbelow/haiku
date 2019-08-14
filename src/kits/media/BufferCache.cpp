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
	for (BufferMap::iterator iterator = fMap.begin(); iterator != fMap.end();
			iterator++) {
		delete iterator->second.buffer;
	}
}


BBuffer*
BufferCache::GetBuffer(media_buffer_id id, port_id port)
{
	if (id <= 0)
		return NULL;

	BufferMap::iterator found = fMap.find(id);
	if (found != fMap.end())
		return found->second.buffer;

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
		fMap.insert(std::make_pair(id, entry));
	} catch (std::bad_alloc& exception) {
		delete buffer;
		return NULL;
	}

	return buffer;
}


void
BufferCache::FlushCacheForPort(port_id port)
{
	for (BufferMap::iterator iterator = fMap.begin(); iterator != fMap.end();) {
		if (iterator->second.port == port) {
			// Delete the buffer
			delete iterator->second.buffer;
			// Then erase it from the map
			iterator = fMap.erase(iterator);
		} else {
			iterator++;
		}
	}
}


}	// namespace BPrivate
