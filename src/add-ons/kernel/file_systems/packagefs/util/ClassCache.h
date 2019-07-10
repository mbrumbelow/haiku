/*
 * Copyright 2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef CLASSCACHE_H
#define CLASSCACHE_H


#include <slab/Slab.h>


template<typename T>
class ClassCache {
public:
	static object_cache* sCache;
	void* operator new(size_t size)
	{
		if (size != sizeof(T))
			panic("unexpected size passed to operator new!");
		if (sCache == NULL) {
			sCache = create_object_cache("packagefs",
				sizeof(T), 8, NULL, NULL, NULL);
		}

		return object_cache_alloc(sCache, 0);
	}

	void operator delete(void* block)
	{
		object_cache_free(sCache, block, 0);
	}
};


template<typename T>
object_cache* ClassCache<T>::sCache = NULL;


#endif	// CLASSCACHE_H
