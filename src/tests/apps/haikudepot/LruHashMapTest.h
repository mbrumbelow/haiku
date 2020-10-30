/*
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#ifndef LRU_HASH_MAP_TEST_H
#define LRU_HASH_MAP_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class LruHashMapTest : public CppUnit::TestCase {
public:
								LruHashMapTest();
	virtual						~LruHashMapTest();

			void				TestAddWithOverflow();
			void				TestAddWithOverflowWithGets();
			void				TestRemove();

	static	void				AddTests(BTestSuite& suite);
};


#endif	// LRU_HASH_MAP_TEST_H
