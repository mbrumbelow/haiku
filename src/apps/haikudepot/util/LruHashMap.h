/*
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef LRU_HASH_MAP_H
#define LRU_HASH_MAP_H


#include <HashMap.h>

#include "Logger.h"


namespace BPrivate {


template<typename Key, typename Value>
class LruOrderingNode {
private:
	typedef LruOrderingNode<Key, Value> LruNode;

public:
	LruOrderingNode()
		:
		fKey(),
		fValue(),
		fOlder(NULL),
		fNewer(NULL)
	{
	}

	LruOrderingNode(const Key& key, const Value& value)
		:
		fKey(key),
		fValue(value),
		fOlder(NULL),
		fNewer(NULL)
	{
	}

	Key						fKey;
	Value					fValue;
	LruNode*				fOlder;
	LruNode*				fNewer;
};


/*!	\brief This is a hash map that maintains a limited number of entries.  Once
	this number of entries has been exceeded then it will start to discard
	entries.  The first entries to be discarded are the ones that are the least
	recently used; hence the prefix "LRU".
*/

template<typename Key, typename Value>
class LruHashMap {
public:
	typedef LruOrderingNode<Key, Value> LruNode;

	LruHashMap(int32 limit)
		:
		fNewestNode(NULL),
		fOldestNode(NULL),
		fLimit(limit)
	{
		if (fLimit < 0)
			fLimit = 0;
	}

	~LruHashMap()
	{
		Clear();
	}

	status_t InitCheck() const
	{
		return fMap.InitCheck();
	}

	status_t Put(const Key& key, const Value& value)
	{
		LruNode* node = fMap.Get(key);

		if (node != NULL) {
			if (node->fValue != value) {
				node->fValue = value;
				_DisconnectNodeAndMakeNewest(node);
			}
		} else {
			node = new(std::nothrow) LruNode(key, value);
			if (node == NULL)
				HDFATAL("memory exhausted adding to an lru hash map");
			status_t result = fMap.Put(key, node);
			if (result != B_OK)
				HDFATAL("failed adding to an lru hash map");
			_SetNewestNode(node);
			_PurgeExcess();
		}

		return B_OK;
	}

	Value Remove(const Key& key)
	{
		LruNode* node = fMap.Get(key);
		if (node != NULL) {
			_DisconnectNode(node);
			Value result = node->fValue;
			fMap.Remove(key);
			delete node;
			return result;
		}
		return Value();
	}

	void Clear()
	{
		fMap.Clear();
		LruNode* node = fNewestNode;
		while (node != NULL) {
			LruNode *next = node->fOlder;
			delete node;
			node = next;
		}
	}

	Value Get(const Key& key)
	{
		LruNode* node = fMap.Get(key);
		if (node != NULL) {
			_DisconnectNodeAndMakeNewest(node);
			return node->fValue;
		}
		return Value();
	}

	bool ContainsKey(const Key& key) const
	{
		return fMap.ContainsKey(key);
	}

	int32 Size() const
	{
		return fMap.Size();
	}

private:

	void _DisconnectNodeAndMakeNewest(LruNode* node) {
		if (node != fNewestNode) {
			_DisconnectNode(node);
			node->fOlder = NULL;
			node->fNewer = NULL;
			_SetNewestNode(node);
		}
	}

	void _DisconnectNode(LruNode* node)
	{
		LruNode *older = node->fOlder;
		LruNode *newer = node->fNewer;
		if (newer != NULL)
			newer->fOlder = older;
		if (older != NULL)
			older->fNewer = newer;
		if (fNewestNode == node)
			fNewestNode = older;
		if (fOldestNode == node)
			fOldestNode = newer;
	}

	void _SetNewestNode(LruNode* node)
	{
		if (node != fNewestNode) {
			node->fOlder = fNewestNode;
			node->fNewer = NULL;
			if (fNewestNode != NULL)
				fNewestNode->fNewer = node;
			fNewestNode = node;
			if (fOldestNode == NULL)
				fOldestNode = node;
		}
	}

	void _PurgeOldestNode()
	{
		if (NULL == fOldestNode)
			HDFATAL("attempt to purge oldest node when there is none to purge");
		Remove(fOldestNode->fKey);
	}

	void _PurgeExcess()
	{
		while(Size() > fLimit)
			_PurgeOldestNode();
	}

protected:
	HashMap<Key, LruNode*>	fMap;
	LruNode*				fNewestNode;
	LruNode*				fOldestNode;

private:
	int32					fLimit;
};

}; // namespace BPrivate

using BPrivate::LruHashMap;

#endif // LRU_HASH_MAP_H
