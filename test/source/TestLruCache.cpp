/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "EASTLTest.h"
#include <EASTL/bonus/lru_cache.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/map.h>
#include <EASTL/string.h>
// #include <EASTL/deque.h>

#ifndef EA_COMPILER_NO_STANDARD_CPP_LIBRARY
	#include <string>
#endif

namespace TestLruCacheInternal
{
	struct Foo
	{
		static int count;

		Foo()
			: a(count++)
			, b(count++)
		{ }

		Foo(int x, int y) : a(x), b(y) {}

		int a;
		int b;

		bool operator==(const Foo &other) const
		{
			return this->a == other.a && this->b == other.b;
		}
	};

	int Foo::count = 0;

	class FooCreator
	{
	public:
		FooCreator() : mFooCreatedCount(0) {}

		Foo	*Create()
		{
			mFooCreatedCount++;
			return new Foo();
		}

		void Destroy(Foo *f)
		{
			delete f;
			mFooCreatedCount--;
		}

		int mFooCreatedCount;
	};

	struct UserKey
	{
		int x;
		int y;

		bool operator==(const UserKey& other) const { return x == other.x && y == other.y; }
		bool operator<(const UserKey& other) const { return x < other.x || (x == other.x && y < other.y); }
	};

	struct UserValue
	{
		eastl::string name;
		int weight = 0;
	};
}

namespace eastl
{
	template <>
	struct hash<TestLruCacheInternal::UserKey>
	{
		size_t operator()(const TestLruCacheInternal::UserKey& k) const
			{ return static_cast<size_t>(k.x) * 31 + static_cast<size_t>(k.y); }
	};
}

// Template instantiations.
// These tell the compiler to compile all the functions for the given class.
template class eastl::lru_cache<int, int>;
template class eastl::lru_cache<int, eastl::string>;
template class eastl::lru_cache<eastl::string, int>;
template class eastl::lru_cache<TestLruCacheInternal::UserKey, TestLruCacheInternal::UserValue>;
template class eastl::lru_cache<int, int, EASTLAllocatorType, eastl::list<int>, eastl::map<int, eastl::pair<int, eastl::list<int>::iterator>>>;
template class eastl::lru_cache<TestLruCacheInternal::UserKey, TestLruCacheInternal::UserValue, EASTLAllocatorType, eastl::list<TestLruCacheInternal::UserKey>,
                                eastl::map<TestLruCacheInternal::UserKey, eastl::pair<TestLruCacheInternal::UserValue, eastl::list<TestLruCacheInternal::UserKey>::iterator>>>;


template<template<typename, typename> typename LruCache>
int TestLruCacheOfType()
{
	int nErrorCount = 0;

	// Test simple situation
	{
		using namespace TestLruCacheInternal;

		Foo::count = 0;

		LruCache<int, Foo> lruCache(3);

		// Empty state
		EATEST_VERIFY(lruCache.contains(1) == false);
		EATEST_VERIFY(lruCache.size() == 0);
		EATEST_VERIFY(lruCache.empty() == true);
		EATEST_VERIFY(lruCache.capacity() == 3);
		EATEST_VERIFY(lruCache.at(1).has_value() == false);

		// Auto create with get call
		EATEST_VERIFY(lruCache[0].a == 0);
		EATEST_VERIFY(lruCache[0].b == 1);
		EATEST_VERIFY(lruCache.contains(1) == false);
		EATEST_VERIFY(lruCache.contains(0) == true);
		EATEST_VERIFY(lruCache.size() == 1);
		EATEST_VERIFY(lruCache.empty() == false);
		EATEST_VERIFY(lruCache.capacity() == 3);

		// Fill structure up to 2 more entries to fill out, also test at()
		lruCache.insert(1, Foo(2, 3));
		EATEST_VERIFY(lruCache.at(1).value().a == 2);
		EATEST_VERIFY(lruCache.at(1).value().b == 3);
		EATEST_VERIFY(lruCache.contains(0) == true);
		EATEST_VERIFY(lruCache.contains(1) == true);
		EATEST_VERIFY(lruCache.contains(2) == false);
		EATEST_VERIFY(lruCache.contains(3) == false);
		EATEST_VERIFY(lruCache.size() == 2);
		EATEST_VERIFY(lruCache.empty() == false);
		EATEST_VERIFY(lruCache.capacity() == 3);

		lruCache.insert(2, Foo(4, 5));
		EATEST_VERIFY(lruCache[2].a == 4);
		EATEST_VERIFY(lruCache[2].b == 5);
		EATEST_VERIFY(lruCache.contains(0) == true);
		EATEST_VERIFY(lruCache.contains(1) == true);
		EATEST_VERIFY(lruCache.contains(2) == true);
		EATEST_VERIFY(lruCache.contains(3) == false);
		EATEST_VERIFY(lruCache.size() == 3);
		EATEST_VERIFY(lruCache.empty() == false);
		EATEST_VERIFY(lruCache.capacity() == 3);

		// Add another entry, at this point 0 is the oldest, so it should be pulled
		lruCache.insert(3, Foo(6, 7));
		EATEST_VERIFY(lruCache[3].a == 6);
		EATEST_VERIFY(lruCache[3].b == 7);
		EATEST_VERIFY(lruCache.contains(0) == false);
		EATEST_VERIFY(lruCache.contains(1) == true);
		EATEST_VERIFY(lruCache.contains(2) == true);
		EATEST_VERIFY(lruCache.contains(3) == true);
		EATEST_VERIFY(lruCache.size() == 3);
		EATEST_VERIFY(lruCache.empty() == false);
		EATEST_VERIFY(lruCache.capacity() == 3);

		// Touch the now oldest 1 key
		EATEST_VERIFY(lruCache.touch(1) == true);

		// Add another entry, this will be #4 but since 1 was touched, 2 is now the oldest
		lruCache.insert(4, Foo(8, 9));
		EATEST_VERIFY(lruCache[4].a == 8);
		EATEST_VERIFY(lruCache[4].b == 9);
		EATEST_VERIFY(lruCache.contains(0) == false);
		EATEST_VERIFY(lruCache.contains(1) == true);
		EATEST_VERIFY(lruCache.contains(2) == false);
		EATEST_VERIFY(lruCache.contains(3) == true);
		EATEST_VERIFY(lruCache.contains(4) == true);
		EATEST_VERIFY(lruCache.size() == 3);
		EATEST_VERIFY(lruCache.empty() == false);
		EATEST_VERIFY(lruCache.capacity() == 3);

		// Test resize down
		EATEST_VERIFY(lruCache.touch(3) == true);	// Let's make some key in the middle the most recent
		lruCache.resize(1);	// Resize down to 1 entry in the cache
		EATEST_VERIFY(lruCache.contains(0) == false);
		EATEST_VERIFY(lruCache.contains(1) == false);
		EATEST_VERIFY(lruCache.contains(2) == false);
		EATEST_VERIFY(lruCache.contains(3) == true);
		EATEST_VERIFY(lruCache.contains(4) == false);
		EATEST_VERIFY(lruCache.size() == 1);
		EATEST_VERIFY(lruCache.empty() == false);
		EATEST_VERIFY(lruCache.capacity() == 1);

		// Let's resize up to a size of 5 now
		lruCache.resize(5);
		EATEST_VERIFY(lruCache.contains(0) == false);
		EATEST_VERIFY(lruCache.contains(1) == false);
		EATEST_VERIFY(lruCache.contains(2) == false);
		EATEST_VERIFY(lruCache.contains(3) == true);
		EATEST_VERIFY(lruCache.contains(4) == false);
		EATEST_VERIFY(lruCache.size() == 1);
		EATEST_VERIFY(lruCache.empty() == false);
		EATEST_VERIFY(lruCache.capacity() == 5);

		// Let's try updating
		lruCache.assign(3, Foo(0, 0));
		EATEST_VERIFY(lruCache[3] == Foo(0, 0));
		EATEST_VERIFY(lruCache.contains(0) == false);
		EATEST_VERIFY(lruCache.contains(1) == false);
		EATEST_VERIFY(lruCache.contains(2) == false);
		EATEST_VERIFY(lruCache.contains(3) == true);
		EATEST_VERIFY(lruCache.contains(4) == false);
		EATEST_VERIFY(lruCache.size() == 1);
		EATEST_VERIFY(lruCache.empty() == false);
		EATEST_VERIFY(lruCache.capacity() == 5);

		// add or update existing
		lruCache.insert_or_assign(3, Foo(1, 1));
		EATEST_VERIFY(lruCache[3] == Foo(1, 1));
		EATEST_VERIFY(lruCache.contains(0) == false);
		EATEST_VERIFY(lruCache.contains(1) == false);
		EATEST_VERIFY(lruCache.contains(2) == false);
		EATEST_VERIFY(lruCache.contains(3) == true);
		EATEST_VERIFY(lruCache.contains(4) == false);
		EATEST_VERIFY(lruCache.size() == 1);
		EATEST_VERIFY(lruCache.empty() == false);
		EATEST_VERIFY(lruCache.capacity() == 5);
		
		// Add or update a new entry
		lruCache.insert_or_assign(25, Foo(2, 2));
		EATEST_VERIFY(lruCache[3] == Foo(1, 1));
		EATEST_VERIFY(lruCache[25] == Foo(2, 2));
		EATEST_VERIFY(lruCache.contains(0) == false);
		EATEST_VERIFY(lruCache.contains(1) == false);
		EATEST_VERIFY(lruCache.contains(2) == false);
		EATEST_VERIFY(lruCache.contains(3) == true);
		EATEST_VERIFY(lruCache.contains(4) == false);
		EATEST_VERIFY(lruCache.contains(25) == true);
		EATEST_VERIFY(lruCache.size() == 2);
		EATEST_VERIFY(lruCache.empty() == false);
		EATEST_VERIFY(lruCache.capacity() == 5);

		// clear everything
		lruCache.clear();
		EATEST_VERIFY(lruCache.size() == 0);
		EATEST_VERIFY(lruCache.empty() == true);
		EATEST_VERIFY(lruCache.capacity() == 5);
		EATEST_VERIFY(lruCache.contains(3) == false);

		// test unilateral reset
		lruCache[1] = Foo(1, 2);
		lruCache.reset_lose_memory();
		EATEST_VERIFY(lruCache.size() == 0);
	}

	// Test more advanced creation / deletion via callbacks
	{
		using namespace TestLruCacheInternal;

		FooCreator fooCreator;

		auto createCallback = [&fooCreator](int) { return fooCreator.Create(); };
		auto deleteCallback = [&fooCreator](Foo *f) { fooCreator.Destroy(f); };

		LruCache<int, Foo*> lruCache(3, EASTLAllocatorType("eastl lru_cache"), createCallback, deleteCallback);

		lruCache[1];
		EATEST_VERIFY(fooCreator.mFooCreatedCount == 1);
		EATEST_VERIFY(lruCache.size() == 1);
		EATEST_VERIFY(lruCache.empty() == false);
		EATEST_VERIFY(lruCache.capacity() == 3);
		EATEST_VERIFY(lruCache.contains(1) == true);
		EATEST_VERIFY(lruCache.contains(2) == false);

		lruCache[2];
		EATEST_VERIFY(fooCreator.mFooCreatedCount == 2);
		EATEST_VERIFY(lruCache.size() == 2);
		EATEST_VERIFY(lruCache.empty() == false);
		EATEST_VERIFY(lruCache.capacity() == 3);
		EATEST_VERIFY(lruCache.contains(1) == true);
		EATEST_VERIFY(lruCache.contains(2) == true);

		// Update 2, which should delete the existing entry
		{
			auto f = fooCreator.Create();
			EATEST_VERIFY(fooCreator.mFooCreatedCount == 3);
			f->a = 20;
			f->b = 21;
			lruCache.assign(2, f);
			EATEST_VERIFY(fooCreator.mFooCreatedCount == 2);
			EATEST_VERIFY(lruCache.size() == 2);
			EATEST_VERIFY(lruCache.empty() == false);
			EATEST_VERIFY(lruCache.capacity() == 3);
			EATEST_VERIFY(lruCache.contains(1) == true);
			EATEST_VERIFY(lruCache.contains(2) == true);
			EATEST_VERIFY(lruCache[2]->a == 20);
			EATEST_VERIFY(lruCache[2]->b == 21);
		}

		lruCache.erase(2);
		EATEST_VERIFY(fooCreator.mFooCreatedCount == 1);
		EATEST_VERIFY(lruCache.size() == 1);
		EATEST_VERIFY(lruCache.empty() == false);
		EATEST_VERIFY(lruCache.capacity() == 3);
		EATEST_VERIFY(lruCache.contains(1) == true);
		EATEST_VERIFY(lruCache.contains(2) == false);

		lruCache.erase(1);
		EATEST_VERIFY(fooCreator.mFooCreatedCount == 0);
		EATEST_VERIFY(lruCache.size() == 0);
		EATEST_VERIFY(lruCache.empty() == true);
		EATEST_VERIFY(lruCache.capacity() == 3);
		EATEST_VERIFY(lruCache.contains(1) == false);
		EATEST_VERIFY(lruCache.contains(2) == false);

		// Test insert_or_assign
		{
			auto f = fooCreator.Create();
			f->a = 22;
			f->b = 30;
			EATEST_VERIFY(fooCreator.mFooCreatedCount == 1);

			lruCache.insert_or_assign(7, f);
			EATEST_VERIFY(lruCache.size() == 1);
			EATEST_VERIFY(lruCache.empty() == false);
			EATEST_VERIFY(lruCache.capacity() == 3);
			EATEST_VERIFY(lruCache.contains(1) == false);
			EATEST_VERIFY(lruCache.contains(2) == false);
			EATEST_VERIFY(lruCache.contains(7) == true);
			EATEST_VERIFY(lruCache.erase(7) == true);
			EATEST_VERIFY(fooCreator.mFooCreatedCount == 0);
		}
	}

	// Test iteration
	{
		LruCache<int, int> lc(5);
		lc.insert_or_assign(0,10);
		lc.insert_or_assign(1,11);
		lc.insert_or_assign(2,12);
		lc.insert_or_assign(3,13);
		lc.insert_or_assign(4,14);

		{ // test manual for-loop
			int i = 0;
			for (auto b = lc.begin(), e = lc.end(); b != e; b++)
			{
				auto &p = *b;
				VERIFY(i == p.first);
				VERIFY(i + 10 == p.second.first);
				i++;
			}
		}

		{ // test pairs 
			int i = 0;
			for(auto& p : lc)
			{
				VERIFY(i == p.first);
				VERIFY(i + 10 == p.second.first);
				i++;
			}
		}

		{ // test structured bindings
			int i = 0;
			for(auto& [key, value] : lc)
			{
				VERIFY(i == key);
				VERIFY(i + 10 == value.first);
				i++;
			}
		}
	}

	// test initializer_list
	{
		LruCache<int, int> lc = {{0, 10}, {1, 11}, {2, 12}, {3, 13}, {4, 14}, {5, 15}};

		int i = 0;
		for(auto& p : lc)
		{
			VERIFY(i == p.first);
			VERIFY(i + 10 == p.second.first);
			i++;
		}
	}

	// emplace
	{
		eastl::lru_cache<int, TestObject> lc = { {0, TestObject{ 1, 2, 3 }}, {1, TestObject{ 4, 5, 6 }} };

		VERIFY((lc.at(0) == TestObject{1, 2, 3}));
		VERIFY((lc.at(1) == TestObject{ 4, 5, 6 }));

		VERIFY((lc.emplace(0, 3, 2, 1).first->second.first == TestObject{ 3, 2, 1 }));
		VERIFY((lc.emplace(1, 6, 5, 4).first->second.first == TestObject{ 6, 5, 4 }));

		// fail to emplace: key 0 already exists.
		auto [ it, did_emplace ] = lc.emplace(0, 9, 9, 9);
		VERIFY(!did_emplace);
		VERIFY((it->second.first == TestObject{ 3, 2, 1 }));
	}

	return nErrorCount;
}

template<typename K, typename V>
using LruCache = eastl::lru_cache<K, V, EASTLAllocatorType, eastl::list<K>, eastl::unordered_map<K, eastl::pair<V, typename eastl::list<K>::iterator>>>;

template<typename K, typename V>
using LruCacheMap = eastl::lru_cache<K, V, EASTLAllocatorType, eastl::list<K>, eastl::map<K, eastl::pair<V, typename eastl::list<K>::iterator>>>;

// template<typename K, typename V>
// using LruCacheDequeMap = eastl::lru_cache<K, V, EASTLAllocatorType, eastl::deque<K>, eastl::map<K, eastl::pair<V, typename eastl::deque<K>::iterator>>>;

int TestLruCache()
{
	int nErrorCount = 0;

	nErrorCount += TestLruCacheOfType<LruCache>();
	nErrorCount += TestLruCacheOfType<LruCacheMap>();
	// could use a deque instead of a list, except that lru_cache requires the container have a reset_lose_memory() function.
	// nErrorCount += TestLruCacheOfType<LruCacheDequeMap>();

#ifndef EA_COMPILER_NO_STANDARD_CPP_LIBRARY
	// emplace with std key and value types: argument-dependent lookup also finds std::make_tuple and std::make_pair.
	{
		using StdStringList = eastl::list<std::string>;
		eastl::lru_cache<std::string, std::string, EASTLAllocatorType, StdStringList,
		                 eastl::unordered_map<std::string, eastl::pair<std::string, StdStringList::iterator>, std::hash<std::string>>> lc(2);

		EATEST_VERIFY(lc.emplace("a", "1").second);

		auto [it, did_emplace] = lc.emplace("a", "2");
		EATEST_VERIFY(!did_emplace);
		EATEST_VERIFY(it->second.first == "1");
	}
#endif

	return nErrorCount;
}