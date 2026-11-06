/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#include <EABase/eabase.h>

// Some versions of GCC generate an array bounds warning in opt builds which 
// doesn't say what line below it comes from and doesn't appear to be a valid 
// warning. In researching this on the Internet it appears that this is a 
// known problem with GCC.
#if defined(EA_DISABLE_GCC_WARNING)
	EA_DISABLE_GCC_WARNING(-Warray-bounds)
#endif

#include "EASTLTest.h"
#include <EASTL/sort.h>
#include <EASTL/bonus/sort_extra.h>
#include <EASTL/bonus/tuple_vector.h>
#include <EASTL/vector.h>
#include <EASTL/list.h>
#include <EASTL/deque.h>
#include <EASTL/algorithm.h>
#include <EASTL/allocator.h>
#include <EASTL/numeric.h>
#include <EASTL/random.h>
#include <EABase/eahave.h>
#include <cmath>


namespace eastl
{
	namespace Internal
	{
		typedef eastl::vector<int>      IntArray;
		typedef eastl::vector<IntArray> IntArrayArray;


		// IntArrayCompare
		// Used to compare IntArray objects.
		struct IntArrayCompare
		{
			bool operator()(const IntArray& a, const IntArray& b)
				{ return a.front() < b.front(); }
		};


		// SafeFloatCompare
		//
		// Float comparison has a problem for the case that either of the floats are a NaN.
		// If you use a NaN in a sort function that uses default floating point comparison then
		// you will get undefined behavior, as all NaNs compare false. This compare function 
		// class sorts floats such that all negative NaNs sort lower than all integers, and all
		// positive NaNs sort higher than all integers.
		//
		// Example usage:
		//     eastl::sort(floatArray.begin(), floatArray.end(), SafeFloatCompare());
		//
		struct SafeFloatCompare
		{
			union FloatInt32{ float f; int32_t i; };

			bool operator()(float a, float b) const
			{
				#if defined(EA_HAVE_ISNAN)
					bool aNan = (EA_HAVE_ISNAN(a) != 0);
					bool bNan = (EA_HAVE_ISNAN(b) != 0);
				#else
					bool aNan = (a != a); // This works as long as the compiler doesn't do any tricks to optimize it away.
					bool bNan = (b != b);
				#endif

				if(!aNan && !bNan)
					return (a < b);

				FloatInt32 fia = { a };
				FloatInt32 fib = { b };

				if(aNan)
				{
					if(bNan)
						return (fia.i < fib.i); // Both are NaNs, so do a binary compare.
					else
						return (fia.i < 0);  // All negative NaNs are assumed to be less than all non-NaNs.
				}
				else
					return (0 < fib.i); // All negative NaNs are assumed to be less than all non-NaNs.
			}
		};



		// StatefulCompare
		// Used to verify that sort<int, StatefulCompare&>() respects the 
		// fact that StatefulCompare is passed by reference instead of by value.
		// All existing commercial STL implementations fail to do what the user
		// wants and instead pass around the compare object by value, even if
		// the user specifically asks to use it by reference. EASTL doesn't 
		// have this problem. 
		struct StatefulCompare
		{
			static int nCtorCount;
			static int nDtorCount;
			static int nCopyCount;

			StatefulCompare()
				{ nCtorCount++; }

			StatefulCompare(StatefulCompare&)
				{ nCopyCount++; }

		   ~StatefulCompare()
				{ nDtorCount++; }

			StatefulCompare& operator=(const StatefulCompare&)
				{ nCopyCount++; return *this; }

			bool operator()(int a, int b)
				{ return a < b; }

			static void Reset()
				{ nCtorCount = 0; nDtorCount = 0; nCopyCount = 0; }
		};

		int StatefulCompare::nCtorCount = 0;
		int StatefulCompare::nDtorCount = 0;
		int StatefulCompare::nCopyCount = 0;


		// TestObjectPtrCompare
		// Used to compare sorted objects by pointer instead of value.
		struct TestObjectPtrCompare
		{
			bool operator()(TestObject* a, TestObject* b)
			   { return a->mX < b->mX; }
		};


		// TestObjectIndexCompare
		// Used to compare sorted objects by array index instead of value.
		struct TestObjectIndexCompare
		{
			vector<TestObject>* mpArray;

			TestObjectIndexCompare(vector<TestObject>* pArray) : mpArray(pArray) { }
			TestObjectIndexCompare(const TestObjectIndexCompare& x)   : mpArray(x.mpArray){ }
			TestObjectIndexCompare& operator=(const TestObjectIndexCompare& x) { mpArray = x.mpArray; return *this; }

			bool operator()(eastl_size_t a, eastl_size_t b)
			   { return (*mpArray)[a] < (*mpArray)[b]; }
		};

		// Radix sort elements
		template <typename Key>
		struct RadixSortElement
		{
			typedef Key radix_type;
			Key  mKey;
			uint16_t mData;

			bool operator<(const RadixSortElement<Key> &other) const
			{
				return mKey < other.mKey;
			}
		};

		typedef RadixSortElement<uint8_t> RadixSortElement8;
		typedef RadixSortElement<uint16_t> RadixSortElement16;
		typedef RadixSortElement<uint32_t> RadixSortElement32;

		struct CountingRadixDataKey
		{
			typedef uint32_t radix_type;
			static int sCallCount;

			radix_type operator()(const RadixSortElement32& value) const
			{
				++sCallCount;
				return value.mData;
			}
		};

		int CountingRadixDataKey::sCallCount = 0;

		template <typename integer_type>
		struct identity_extract_radix_key
		{
			typedef integer_type radix_type;

			const radix_type operator()(const integer_type& x) const
			{
				return x;
			}
		};
	} // namespace Internal

} // namespace eastl

int TestSort()
{
	using namespace eastl;
	using namespace Internal;

	int nErrorCount = 0;

	EASTLTest_Rand rng(EA::UnitTest::GetRandSeed());

	// shell_sort, heap_sort, comb_sort, partial_sort, radix_sort, bucket_sort, selection_sort, bubble_sort and
	// shaker_sort on many sizes and input orders: the result holds each element once, in order of key, and the stable
	// sorts (radix_sort, bucket_sort, bubble_sort, shaker_sort) keep equal keys in their original order.
	{
		struct Item { int mKey; int mIndex; };
		struct ItemRadixKey { typedef uint32_t radix_type; uint32_t operator()(const Item& item) const { return (uint32_t)item.mKey; } };
		struct ItemBucket { int operator()(const Item& item) const { return item.mKey; } };
		auto keyLess = [](const Item& a, const Item& b) { return a.mKey < b.mKey; };
		auto keyOf = [](int order, int size, int i)
		{
			switch(order)
			{
				case 0:  return (i * 7919) % 97;                       // Scattered, with duplicates.
				case 1:  return i;                                     // Sorted.
				case 2:  return size - i;                              // Reversed.
				case 3:  return i % 7;                                 // Sawtooth.
				case 4:  return (i < (size / 2)) ? i : (size - i);     // Organ pipe.
				case 5:  return 3;                                     // All equal.
				case 6:  return (i * 31) % 3;                          // Three values.
				default: return ((i % 50) == 7) ? ((i * 7919) % 97) : i; // Nearly sorted.
			}
		};

		// Whether items holds each element of the input once, in order of key in [0, nSorted), with no key after nSorted
		// less than the last of those, and, if bStable, equal keys in their original order.
		auto isSorted = [&](const vector<Item>& items, int order, int size, int nSorted, bool bStable)
		{
			if((int)items.size() != size)
				return false;

			vector<int> seen(size, 0);
			for(int i = 0; i < size; i++)
			{
				const Item& item = items[i];
				if((item.mIndex < 0) || (item.mIndex >= size) || (seen[item.mIndex]++ != 0) || (item.mKey != keyOf(order, size, item.mIndex)))
					return false;

				if((i > 0) && (i < nSorted))
				{
					if(item.mKey < items[i - 1].mKey)
						return false;
					if(bStable && (item.mKey == items[i - 1].mKey) && (item.mIndex < items[i - 1].mIndex))
						return false;
				}

				if((i >= nSorted) && (nSorted > 0) && (item.mKey < items[nSorted - 1].mKey))
					return false;
			}
			return true;
		};

		const int sizes[] = { 0, 1, 2, 3, 15, 16, 17, 31, 32, 33, 63, 64, 65, 100, 257, 1000 };
		for(int size : sizes)
		{
			for(int order = 0; order < 8; order++)
			{
				vector<Item> items;
				for(int i = 0; i < size; i++)
					items.push_back(Item{keyOf(order, size, i), i});

				vector<Item> a(items);
				shell_sort(a.begin(), a.end(), keyLess);
				EATEST_VERIFY(isSorted(a, order, size, size, false));

				a = items;
				heap_sort(a.begin(), a.end(), keyLess);
				EATEST_VERIFY(isSorted(a, order, size, size, false));

				a = items;
				comb_sort(a.begin(), a.end(), keyLess);
				EATEST_VERIFY(isSorted(a, order, size, size, false));

				a = items;
				partial_sort(a.begin(), a.end(), a.end(), keyLess);
				EATEST_VERIFY(isSorted(a, order, size, size, false));

				a = items;
				partial_sort(a.begin(), a.begin() + size / 3, a.end(), keyLess);
				EATEST_VERIFY(isSorted(a, order, size, size / 3, false));

				a = items;
				vector<Item> buffer(size);
				radix_sort<Item*, ItemRadixKey>(a.begin(), a.end(), buffer.begin());
				EATEST_VERIFY(isSorted(a, order, size, size, true));

				a = items;
				vector<vector<Item>> buckets(size + 98); // Keys are at most the larger of size and 96.
				bucket_sort(a.begin(), a.end(), buckets, ItemBucket());
				EATEST_VERIFY(isSorted(a, order, size, size, true));

				if(size <= 257)
				{
					a = items;
					selection_sort(a.begin(), a.end(), keyLess);
					EATEST_VERIFY(isSorted(a, order, size, size, false));

					a = items;
					bubble_sort(a.begin(), a.end(), keyLess);
					EATEST_VERIFY(isSorted(a, order, size, size, true));

					a = items;
					shaker_sort(a.begin(), a.end(), keyLess);
					EATEST_VERIFY(isSorted(a, order, size, size, true));
				}
			}
		}
	}

	{
		// is_sorted
		int array[] = { 0, 1, 2, 2, 2, 3, 4, 5, 6, 7, 8, 9, 9, 8, 7, 6, 5, 4, 3, 2, 2, 2, 1, 0 };
	
		EATEST_VERIFY( is_sorted(array +  0, array +  0));
		EATEST_VERIFY( is_sorted(array +  2, array +  4));
		EATEST_VERIFY( is_sorted(array +  0, array + 10));
		EATEST_VERIFY(!is_sorted(array +  0, array + 14));
		EATEST_VERIFY( is_sorted(array + 11, array + 23, eastl::greater<int>()));
	}

	{
		// is_sorted_until
		int sorted[]    = { 0, 1, 2, 3, 4,  5, 6, 7, 8, 9 };
		int notsorted[] = { 0, 1, 2, 3, 4, 42, 6, 7, 8, 9 };
			
		EATEST_VERIFY( is_sorted_until(sorted + EAArrayCount(sorted),  sorted + EAArrayCount(sorted)) == sorted + EAArrayCount(sorted) );
		EATEST_VERIFY( is_sorted_until(sorted                       ,  sorted + EAArrayCount(sorted)) == sorted + EAArrayCount(sorted) );

		EATEST_VERIFY( is_sorted_until(sorted +  0, sorted +  0) == sorted     );
		EATEST_VERIFY( is_sorted_until(sorted +  2, sorted +  8) == sorted + 8 );

		EATEST_VERIFY( is_sorted_until(notsorted +  2, notsorted +  8) == notsorted + 6 );
		
		// is_sorted_until (with compare function)
		EATEST_VERIFY( is_sorted_until(sorted + EAArrayCount(sorted),  sorted + EAArrayCount(sorted), eastl::less<int>()) == sorted + EAArrayCount(sorted) );
		EATEST_VERIFY( is_sorted_until(notsorted +  2, notsorted +  8, eastl::less<int>()) == notsorted + 6 );
	}

	// Sort arrays of size 0 - N. Sort M random permutations of each.
	{
		vector<int64_t> intArray, intArraySaved;

		for(int i = 0; i < (150 + (gEASTL_TestLevel * 200)); i += (i < 5) ? 1 : 37) // array sizes of 0 to 300 - 2100, depending on test level.
		{
			// intArraySaved.clear(); // Do we want to do this?

			for(int n = 0; n < i; n++)
			{
				intArraySaved.push_back(n);

				if(rng.RandLimit(10) == 0)
				{
					intArraySaved.push_back(n);

					if(rng.RandLimit(5) == 0)
						intArraySaved.push_back(n);
				}
			}
			const int64_t expectedSum = eastl::accumulate(begin(intArraySaved), end(intArraySaved), int64_t(0));

			for(int j = 0; j < 300 + (gEASTL_TestLevel * 50); j++)
			{
				eastl::random_shuffle(intArraySaved.begin(), intArraySaved.end(), rng);

				intArray = intArraySaved;
				bubble_sort(intArray.begin(), intArray.end());
				EATEST_VERIFY(is_sorted(intArray.begin(), intArray.end()));
				EATEST_VERIFY(eastl::accumulate(begin(intArraySaved), end(intArraySaved), int64_t(0)) == expectedSum);

				intArray = intArraySaved;
				shaker_sort(intArray.begin(), intArray.end());
				EATEST_VERIFY(is_sorted(intArray.begin(), intArray.end()));
				EATEST_VERIFY(eastl::accumulate(begin(intArraySaved), end(intArraySaved), int64_t(0)) == expectedSum);

				intArray = intArraySaved;
				insertion_sort(intArray.begin(), intArray.end());
				EATEST_VERIFY(is_sorted(intArray.begin(), intArray.end()));
				EATEST_VERIFY(eastl::accumulate(begin(intArraySaved), end(intArraySaved), int64_t(0)) == expectedSum);

				intArray = intArraySaved;
				selection_sort(intArray.begin(), intArray.end());
				EATEST_VERIFY(is_sorted(intArray.begin(), intArray.end()));
				EATEST_VERIFY(eastl::accumulate(begin(intArraySaved), end(intArraySaved), int64_t(0)) == expectedSum);

				intArray = intArraySaved;
				shell_sort(intArray.begin(), intArray.end());
				EATEST_VERIFY(is_sorted(intArray.begin(), intArray.end()));
				EATEST_VERIFY(eastl::accumulate(begin(intArraySaved), end(intArraySaved), int64_t(0)) == expectedSum);

				intArray = intArraySaved;
				comb_sort(intArray.begin(), intArray.end());
				EATEST_VERIFY(is_sorted(intArray.begin(), intArray.end()));
				EATEST_VERIFY(eastl::accumulate(begin(intArraySaved), end(intArraySaved), int64_t(0)) == expectedSum);

				intArray = intArraySaved;
				heap_sort(intArray.begin(), intArray.end());
				EATEST_VERIFY(is_sorted(intArray.begin(), intArray.end()));
				EATEST_VERIFY(eastl::accumulate(begin(intArraySaved), end(intArraySaved), int64_t(0)) == expectedSum);

				intArray = intArraySaved;
				merge_sort(intArray.begin(), intArray.end(), *get_default_allocator((EASTLAllocatorType*)NULL));
				EATEST_VERIFY(is_sorted(intArray.begin(), intArray.end()));
				EATEST_VERIFY(eastl::accumulate(begin(intArraySaved), end(intArraySaved), int64_t(0)) == expectedSum);

				intArray = intArraySaved;
				vector<int64_t> buffer(intArray.size());
				merge_sort_buffer(intArray.begin(), intArray.end(), buffer.data());
				EATEST_VERIFY(is_sorted(intArray.begin(), intArray.end()));
				EATEST_VERIFY(eastl::accumulate(begin(intArraySaved), end(intArraySaved), int64_t(0)) == expectedSum);

				intArray = intArraySaved;
				quick_sort(intArray.begin(), intArray.end());
				EATEST_VERIFY(is_sorted(intArray.begin(), intArray.end()));
				EATEST_VERIFY(eastl::accumulate(begin(intArraySaved), end(intArraySaved), int64_t(0)) == expectedSum);

				intArray = intArraySaved;
				buffer.resize(intArray.size()/2);
				tim_sort_buffer(intArray.begin(), intArray.end(), buffer.data());
				EATEST_VERIFY(is_sorted(intArray.begin(), intArray.end()));
				EATEST_VERIFY(eastl::accumulate(begin(intArraySaved), end(intArraySaved), int64_t(0)) == expectedSum);
			}
		}
	}

	// Test tim sort with a specific array size and seed that caused a crash 
	{
		vector<int64_t> intArray;
		int i = 1000000;
		{
			EASTLTest_Rand testRng(232526);

			for (int n = 0; n < i; n++)
			{
				intArray.push_back(testRng.Rand());
			}
			vector<int64_t> buffer(intArray.size() / 2);
			tim_sort_buffer(intArray.begin(), intArray.end(), buffer.data());
			EATEST_VERIFY(is_sorted(intArray.begin(), intArray.end()));
		}
	}

	// Test insertion_sort() does not invalidate a BidirectionalIterator by doing --BidirectionalIterator.begin()
	{
		// Test Passes if the Test doesn't crash
		eastl::deque<int> deque;
		deque.push_back(1);

		insertion_sort(deque.begin(), deque.end());

		insertion_sort(deque.begin(), deque.end(), eastl::less<int>{});
	}


	// TestObject sorting
	TestObject::Reset();
	{
		vector<TestObject> toArray, toArraySaved;

		for(int i = 0; i < (150 + (gEASTL_TestLevel * 200)); i += (i < 5) ? 1 : 37) // array sizes of 0 to 300 - 2100, depending on test level.
		{
			for(int n = 0; n < i; n++)
			{
				toArraySaved.push_back(TestObject(n));

				if(rng.RandLimit(10) == 0)
				{
					toArraySaved.push_back(TestObject(n));

					if(rng.RandLimit(5) == 0)
						toArraySaved.push_back(TestObject(n));
				}
			}

			for(int j = 0; j < 300 + (gEASTL_TestLevel * 50); j++)
			{
				eastl::random_shuffle(toArraySaved.begin(), toArraySaved.end(), rng);

				toArray = toArraySaved;
				bubble_sort(toArray.begin(), toArray.end());
				EATEST_VERIFY(is_sorted(toArray.begin(), toArray.end()));

				toArray = toArraySaved;
				shaker_sort(toArray.begin(), toArray.end());
				EATEST_VERIFY(is_sorted(toArray.begin(), toArray.end()));

				toArray = toArraySaved;
				insertion_sort(toArray.begin(), toArray.end());
				EATEST_VERIFY(is_sorted(toArray.begin(), toArray.end()));

				toArray = toArraySaved;
				selection_sort(toArray.begin(), toArray.end());
				EATEST_VERIFY(is_sorted(toArray.begin(), toArray.end()));

				toArray = toArraySaved;
				shell_sort(toArray.begin(), toArray.end());
				EATEST_VERIFY(is_sorted(toArray.begin(), toArray.end()));

				toArray = toArraySaved;
				comb_sort(toArray.begin(), toArray.end());
				EATEST_VERIFY(is_sorted(toArray.begin(), toArray.end()));

				toArray = toArraySaved;
				heap_sort(toArray.begin(), toArray.end());
				EATEST_VERIFY(is_sorted(toArray.begin(), toArray.end()));

				// Not ready yet:
				toArray = toArraySaved;
				merge_sort(toArray.begin(), toArray.end(), *get_default_allocator((EASTLAllocatorType*)NULL));
				EATEST_VERIFY(is_sorted(toArray.begin(), toArray.end()));

				toArray = toArraySaved;
				quick_sort(toArray.begin(), toArray.end());
				EATEST_VERIFY(is_sorted(toArray.begin(), toArray.end()));

				toArray = toArraySaved;
				vector<TestObject> buffer(toArray.size()/2);
				tim_sort_buffer(toArray.begin(), toArray.end(), buffer.data());
				EATEST_VERIFY(is_sorted(toArray.begin(), toArray.end()));
			}
		}
	}

	// Test that stable sorting algorithms are actually stable
	{
		struct StableSortTestObj
		{
			StableSortTestObj()
			{
			}

			StableSortTestObj(int value)
				:value(value)
				,initialPositionIndex(0)
			{
			}

			int value;
			size_t initialPositionIndex;
		};

		// During the test this comparison is used to sort elements based on value.
		struct StableSortCompare
		{
			bool operator()(const StableSortTestObj& a, const StableSortTestObj& b)
			{
				return a.value < b.value;
			}
		};

		// During the test this comparison is used to verify the sort was a stable sort.  i.e. if values are the same then
		// their relative position should be maintained.
		struct StableSortCompareForStability
		{
			bool operator()(const StableSortTestObj& a, const StableSortTestObj& b)
			{
				if (a.value != b.value)
				{
					return a.value < b.value;
				}
				else
				{
					return a.initialPositionIndex < b.initialPositionIndex;
				}
			}
		};

		vector<StableSortTestObj> toArray, toArraySaved;
		StableSortCompare compare;
		StableSortCompareForStability compareForStability;

		for (int i = 0; i < (150 + (gEASTL_TestLevel * 200)); i += (i < 5) ? 1 : 37) // array sizes of 0 to 300 - 2100, depending on test level.
		{
			for (int n = 0; n < i; n++)
			{
				toArraySaved.push_back(StableSortTestObj(n));

				if (rng.RandLimit(10) == 0)
				{
					toArraySaved.push_back(StableSortTestObj(n));

					if (rng.RandLimit(5) == 0)
						toArraySaved.push_back(StableSortTestObj(n));
				}
			}
			vector<StableSortTestObj> tempBuffer;
			tempBuffer.resize(toArraySaved.size());

			for (int j = 0; j < 300 + (gEASTL_TestLevel * 50); j++)
			{
				eastl::random_shuffle(toArraySaved.begin(), toArraySaved.end(), rng);
				// Store the intial position of each element in the array before sorting.  This position can then be used to verify that the sorting operation is stable.
				for (vector<StableSortTestObj>::size_type k = 0; k < toArraySaved.size(); k++)
				{
					toArraySaved[k].initialPositionIndex = k;
				}

				toArray = toArraySaved;
				bubble_sort(toArray.begin(), toArray.end(), compare);
				EATEST_VERIFY(is_sorted(toArray.begin(), toArray.end(), compareForStability));

				toArray = toArraySaved;
				shaker_sort(toArray.begin(), toArray.end(), compare);
				EATEST_VERIFY(is_sorted(toArray.begin(), toArray.end(), compareForStability));

				toArray = toArraySaved;
				insertion_sort(toArray.begin(), toArray.end(), compare);
				EATEST_VERIFY(is_sorted(toArray.begin(), toArray.end(), compareForStability));

				toArray = toArraySaved;
				tim_sort_buffer(toArray.begin(), toArray.end(), tempBuffer.data(), compare);
				EATEST_VERIFY(is_sorted(toArray.begin(), toArray.end(), compareForStability));

				toArray = toArraySaved;
				merge_sort(toArray.begin(), toArray.end(), *get_default_allocator((EASTLAllocatorType*)NULL), compare);
				EATEST_VERIFY(is_sorted(toArray.begin(), toArray.end(), compareForStability));

				toArray = toArraySaved;
				merge_sort_buffer(toArray.begin(), toArray.end(), tempBuffer.data(), compare);
				EATEST_VERIFY(is_sorted(toArray.begin(), toArray.end(), compareForStability));
			}
		}
	}

	{
		// OutputIterator merge(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, OutputIterator result)
		// This is tested by merge_sort.
	}


	{
		// void partial_sort(RandomAccessIterator first, RandomAccessIterator middle, RandomAccessIterator last)
		// This is tested by quick_sort.
	}


	{
		// void nth_element(RandomAccessIterator first, RandomAccessIterator nth, RandomAccessIterator last)
		// void nth_element(RandomAccessIterator first, RandomAccessIterator nth, RandomAccessIterator last, Compare compare)
		const int intArrayInit[16] = { 4, 2, 8, 6, 9, 1, 1, 4, 0, 5, 5, 7, 8, 9, 3, 3 };
		int intArraySorted[16]; // Same as intArrayInit but sorted
		int intArray[16];
		size_t i, j;

		// We test many combinations of nth_element on the int array.
		for(i = 1; i < 16; i++)
		{
			for(j = 0; j < i; j++)
			{
				eastl::copy(intArrayInit, intArrayInit + i, intArraySorted);
				eastl::sort(intArraySorted, intArraySorted + i);

				eastl::copy(intArrayInit, intArrayInit + i, intArray);
				nth_element(intArray, intArray + j, intArray + i);
				EATEST_VERIFY(intArray[j] == intArraySorted[j]);
			}
		}

		for(i = 1; i < 16; i++)
		{
			for(j = 0; j < i; j++)
			{
				eastl::copy(intArrayInit, intArrayInit + i, intArraySorted);
				eastl::sort(intArraySorted, intArraySorted + i);

				eastl::copy(intArrayInit, intArrayInit + 16, intArray);
				nth_element(intArray, intArray + j, intArray + i, eastl::less<int>());
				EATEST_VERIFY(intArray[j] == intArraySorted[j]);
			}
		}
	}


	{
		// void radix_sort(RandomAccessIterator first, RandomAccessIterator last, RandomAccessIterator buffer);
		const uint32_t kCount = 100;

		{
			// Keep valid sentinels behind an empty range so an unwanted key extraction is observable without an invalid read.
			RadixSortElement32 input[] = { { 37, 3 } };
			RadixSortElement32 buffer[] = { { 91, 8 } };
			CountingRadixDataKey::sCallCount = 0;
			radix_sort<RadixSortElement32*, CountingRadixDataKey>(input, input, buffer);
			radix_sort<RadixSortElement32*, CountingRadixDataKey, 3>(input, input, buffer);
			EATEST_VERIFY(CountingRadixDataKey::sCallCount == 0);
			EATEST_VERIFY(input[0].mKey == 37 && input[0].mData == 3);
			EATEST_VERIFY(buffer[0].mKey == 91 && buffer[0].mData == 8);

			radix_sort<RadixSortElement32*, CountingRadixDataKey>(input, input + 1, buffer);
			EATEST_VERIFY(CountingRadixDataKey::sCallCount > 0);
			EATEST_VERIFY(input[0].mKey == 37 && input[0].mData == 3);
		}

		{
			RadixSortElement32* pRadixSortElementArray32     = new RadixSortElement32[kCount];
			RadixSortElement32* pRadixSortElementArrayTemp32 = new RadixSortElement32[kCount];
			for(uint32_t i = 0; i < kCount; i++)
			{
				pRadixSortElementArray32[i].mKey  = (uint16_t)(kCount - i);
				pRadixSortElementArray32[i].mData = (uint16_t)i;
			}
			radix_sort<RadixSortElement32*, extract_radix_key<RadixSortElement32> >(pRadixSortElementArray32, pRadixSortElementArray32 + kCount, pRadixSortElementArrayTemp32);
			EATEST_VERIFY(is_sorted(pRadixSortElementArray32, pRadixSortElementArray32 + kCount));
			delete[] pRadixSortElementArray32;
			delete[] pRadixSortElementArrayTemp32;
		}

		{
			RadixSortElement16* pRadixSortElementArray16     = new RadixSortElement16[kCount];
			RadixSortElement16* pRadixSortElementArrayTemp16 = new RadixSortElement16[kCount];
			for(uint32_t i = 0; i < kCount; i++)
			{
				pRadixSortElementArray16[i].mKey  = (uint16_t)(kCount - i);
				pRadixSortElementArray16[i].mData = (uint16_t)i;
			}
			radix_sort<RadixSortElement16*, extract_radix_key<RadixSortElement16> >(pRadixSortElementArray16, pRadixSortElementArray16 + kCount, pRadixSortElementArrayTemp16);
			EATEST_VERIFY(is_sorted(pRadixSortElementArray16, pRadixSortElementArray16 + kCount));
			delete[] pRadixSortElementArray16;
			delete[] pRadixSortElementArrayTemp16;
		}

		{
			RadixSortElement8* pRadixSortElementArray8     = new RadixSortElement8[kCount];
			RadixSortElement8* pRadixSortElementArrayTemp8 = new RadixSortElement8[kCount];
			for(uint32_t i = 0; i < kCount; i++)
			{
				pRadixSortElementArray8[i].mKey  = (uint8_t)(kCount - i);
				pRadixSortElementArray8[i].mData = (uint8_t)i;
			}
			radix_sort<RadixSortElement8*, extract_radix_key<RadixSortElement8> >(pRadixSortElementArray8, pRadixSortElementArray8 + kCount, pRadixSortElementArrayTemp8);
			EATEST_VERIFY(is_sorted(pRadixSortElementArray8, pRadixSortElementArray8 + kCount));
			delete[] pRadixSortElementArray8;
			delete[] pRadixSortElementArrayTemp8;
		}
	}

	{
		// Do some white-box testing of radix sort to verify internal optimizations work properly for some edge cases.

		{
			uint32_t input[] = { 123, 15, 76, 2, 74, 12, 62, 91 };
			uint32_t buffer[EAArrayCount(input)];
			radix_sort<uint32_t*, identity_extract_radix_key<uint32_t>>(begin(input), end(input), buffer);
			EATEST_VERIFY(is_sorted(begin(input), end(input)));
		}
		{
			// Test values where some digit positions have identical values
			uint32_t input[] = { 0x75000017, 0x74000003, 0x73000045, 0x76000024, 0x78000033, 0x76000099, 0x78000043, 0x75000010 };
			uint32_t buffer[EAArrayCount(input)];
			radix_sort<uint32_t*, identity_extract_radix_key<uint32_t>>(begin(input), end(input), buffer);
			EATEST_VERIFY(is_sorted(begin(input), end(input)));
		}
		{
			// Test values where some digit positions have identical values
			uint32_t input[] = { 0x00750017, 0x00740003, 0x00730045, 0x00760024, 0x00780033, 0x00760099, 0x00780043, 0x00750010 };
			uint32_t buffer[EAArrayCount(input)];
			radix_sort<uint32_t*, identity_extract_radix_key<uint32_t>>(begin(input), end(input), buffer);
			EATEST_VERIFY(is_sorted(begin(input), end(input)));
		}
		{
			// Test values where an odd number of scatter operations will be done during sorting (which forces a copy operation to move values back to the input buffer).
			uint32_t input[] = { 0x00000017, 0x00000003, 0x00000045, 0x00000024, 0x00000033, 0x00000099, 0x00000043, 0x00000010 };
			uint32_t buffer[EAArrayCount(input)];
			radix_sort<uint32_t*, identity_extract_radix_key<uint32_t>>(begin(input), end(input), buffer);
			EATEST_VERIFY(is_sorted(begin(input), end(input)));
		}
		{
			// Test case for bug where the last histogram bucket was not being cleared to zero
			uint32_t input[] = { 0xff00, 0xff };
			uint32_t buffer[EAArrayCount(input)];
			radix_sort<uint32_t*, identity_extract_radix_key<uint32_t>>(begin(input), end(input), buffer);
			EATEST_VERIFY(is_sorted(begin(input), end(input)));
		}
	}

	{
		// Test different values for DigitBits

		{
			uint32_t input[] = {2514513, 6278225, 2726217, 963245656, 35667326, 2625624562, 3562562562, 1556256252};
			uint32_t buffer[EAArrayCount(input)];
			radix_sort<uint32_t*, identity_extract_radix_key<uint32_t>, 1>(begin(input), end(input), buffer);
			EATEST_VERIFY(is_sorted(begin(input), end(input)));
		}
		{
			uint32_t input[] = { 2514513, 6278225, 2726217, 963245656, 35667326, 2625624562, 3562562562, 1556256252 };
			uint32_t buffer[EAArrayCount(input)];
			radix_sort<uint32_t*, identity_extract_radix_key<uint32_t>, 3>(begin(input), end(input), buffer);
			EATEST_VERIFY(is_sorted(begin(input), end(input)));
		}
		{
			uint32_t input[] = { 2514513, 6278225, 2726217, 963245656, 35667326, 2625624562, 3562562562, 1556256252 };
			uint32_t buffer[EAArrayCount(input)];
			radix_sort<uint32_t*, identity_extract_radix_key<uint32_t>, 6>(begin(input), end(input), buffer);
			EATEST_VERIFY(is_sorted(begin(input), end(input)));
		}
		{
			// Test a value for DigitBits that is more than half the size of the type.
			uint16_t input[] = { 14513, 58225, 26217, 34656, 63326, 24562, 35562, 15652 };
			uint16_t buffer[EAArrayCount(input)];
			radix_sort<uint16_t*, identity_extract_radix_key<uint16_t>, 11>(begin(input), end(input), buffer);
			EATEST_VERIFY(is_sorted(begin(input), end(input)));
		}
		{
			// Test a value for DigitBits that is the size of the type itself.
			uint8_t input[] = { 113, 225, 217, 56, 26, 162, 62, 152 };
			uint8_t buffer[EAArrayCount(input)];
			radix_sort<uint8_t*, identity_extract_radix_key<uint8_t>, 8>(begin(input), end(input), buffer);
			EATEST_VERIFY(is_sorted(begin(input), end(input)));
		}

	}

	{
		// void bucket_sort(ForwardIterator first, ForwardIterator last, ContainerArray& bucketArray, HashFunction hash)

		const size_t kElementRange = 32;
		vector<int>  intArray(1000);

		for(int i = 0; i < 1000; i++)
			intArray[i] = rng() % kElementRange;

		vector< vector<int> > bucketArray(kElementRange);
		bucket_sort(intArray.begin(), intArray.end(), bucketArray, eastl::hash_use_self<int>());
		EATEST_VERIFY(is_sorted(intArray.begin(), intArray.end()));
	}


	{ 
		// stable_sort general test
		typedef eastl::less<int> IntCompare;

		int intArray[2] = { 0, 1 };

		stable_sort(intArray, intArray + 2);
		stable_sort(intArray, intArray + 2, IntCompare());
		stable_sort<int*>(intArray, intArray + 2);
		stable_sort<int*, IntCompare>(intArray, intArray + 2, IntCompare());

		MallocAllocator mallocAllocator;

		//stable_sort(intArray, intArray + 2, mallocAllocator);
		stable_sort(intArray, intArray + 2, mallocAllocator, IntCompare());
		//stable_sort<int*, MallocAllocator>(intArray, intArray + 2, mallocAllocator);
		stable_sort<int*, MallocAllocator, IntCompare>(intArray, intArray + 2, mallocAllocator, IntCompare());
	}

	{
		// stable_sort special test
		IntArrayArray   intArrayArray(2);
		IntArrayCompare compare;

		intArrayArray[0].push_back(0);
		intArrayArray[1].push_back(1);
		 
		stable_sort(intArrayArray.begin(), intArrayArray.end(), compare);
	}


	{
		// Test to verify that Compare object references are preserved.
		typedef deque<int>         IntDeque;
		typedef IntDeque::iterator IntDequeIterator;

		IntDeque        intDeque, intDequeSaved;
		StatefulCompare compare;

		// Set up intDequeSaved with random data.
		for(int n = 0; n < 500; n++)
		{
			intDequeSaved.push_back(n);

			if(rng.RandLimit(10) == 0)
			{
				intDequeSaved.push_back(n);

				if(rng.RandLimit(5) == 0)
					intDequeSaved.push_back(n);
			}
		}

		eastl::random_shuffle(intDequeSaved.begin(), intDequeSaved.end(), rng);

		StatefulCompare::Reset();
		intDeque = intDequeSaved;
		bubble_sort<IntDequeIterator, StatefulCompare&>(intDeque.begin(), intDeque.end(), compare);
		EATEST_VERIFY((StatefulCompare::nCtorCount == 0) && (StatefulCompare::nDtorCount == 0) && (StatefulCompare::nCopyCount == 0));

		StatefulCompare::Reset();
		intDeque = intDequeSaved;
		shaker_sort<IntDequeIterator, StatefulCompare&>(intDeque.begin(), intDeque.end(), compare);
		EATEST_VERIFY((StatefulCompare::nCtorCount == 0) && (StatefulCompare::nDtorCount == 0) && (StatefulCompare::nCopyCount == 0));

		StatefulCompare::Reset();
		intDeque = intDequeSaved;
		insertion_sort<IntDequeIterator, StatefulCompare&>(intDeque.begin(), intDeque.end(), compare);
		EATEST_VERIFY((StatefulCompare::nCtorCount == 0) && (StatefulCompare::nDtorCount == 0) && (StatefulCompare::nCopyCount == 0));

		StatefulCompare::Reset();
		intDeque = intDequeSaved;
		selection_sort<IntDequeIterator, StatefulCompare&>(intDeque.begin(), intDeque.end(), compare);
		EATEST_VERIFY((StatefulCompare::nCtorCount == 0) && (StatefulCompare::nDtorCount == 0) && (StatefulCompare::nCopyCount == 0));

		StatefulCompare::Reset();
		intDeque = intDequeSaved;
		shell_sort<IntDequeIterator, StatefulCompare&>(intDeque.begin(), intDeque.end(), compare);
		EATEST_VERIFY((StatefulCompare::nCtorCount == 0) && (StatefulCompare::nDtorCount == 0) && (StatefulCompare::nCopyCount == 0));

		StatefulCompare::Reset();
		intDeque = intDequeSaved;
		comb_sort<IntDequeIterator, StatefulCompare&>(intDeque.begin(), intDeque.end(), compare);
		EATEST_VERIFY((StatefulCompare::nCtorCount == 0) && (StatefulCompare::nDtorCount == 0) && (StatefulCompare::nCopyCount == 0));

		StatefulCompare::Reset();
		intDeque = intDequeSaved;
		heap_sort<IntDequeIterator, StatefulCompare&>(intDeque.begin(), intDeque.end(), compare);
		EATEST_VERIFY((StatefulCompare::nCtorCount == 0) && (StatefulCompare::nDtorCount == 0) && (StatefulCompare::nCopyCount == 0));

		StatefulCompare::Reset();
		intDeque = intDequeSaved;
		merge_sort<IntDequeIterator, EASTLAllocatorType, StatefulCompare&>(intDeque.begin(), intDeque.end(), *get_default_allocator((EASTLAllocatorType*)NULL), compare);
		EATEST_VERIFY((StatefulCompare::nCtorCount == 0) && (StatefulCompare::nDtorCount == 0) && (StatefulCompare::nCopyCount == 0));

		StatefulCompare::Reset();
		intDeque = intDequeSaved;
		quick_sort<IntDequeIterator, StatefulCompare&>(intDeque.begin(), intDeque.end(), compare);
		EATEST_VERIFY((StatefulCompare::nCtorCount == 0) && (StatefulCompare::nDtorCount == 0) && (StatefulCompare::nCopyCount == 0));

		StatefulCompare::Reset();
		vector<int> buffer(intDeque.size()/2);
		intDeque = intDequeSaved;
		tim_sort_buffer<IntDequeIterator, int, StatefulCompare&>(intDeque.begin(), intDeque.end(), buffer.data(), compare);
		EATEST_VERIFY((StatefulCompare::nCtorCount == 0) && (StatefulCompare::nDtorCount == 0) && (StatefulCompare::nCopyCount == 0));
	}

	{
		// EATEST_VERIFY deque sorting can compile.
		deque<int>  intDeque;
		vector<int> intVector;

		stable_sort(intDeque.begin(),  intDeque.end());
		stable_sort(intVector.begin(), intVector.end());
	}


	{
		// Test sorting of a container of pointers to objects as opposed to a container of objects themselves.
		vector<TestObject>  toArray;
		vector<TestObject*> topArray;

		for(eastl_size_t i = 0; i < 32; i++)
			toArray.push_back(TestObject((int)rng.RandLimit(20)));
		for(eastl_size_t i = 0; i < 32; i++) // This needs to be a second loop because the addresses might change in the first loop due to container resizing.
			topArray.push_back(&toArray[i]);

		quick_sort(topArray.begin(), topArray.end(), TestObjectPtrCompare());
		EATEST_VERIFY(is_sorted(topArray.begin(), topArray.end(), TestObjectPtrCompare()));
	}


	{
		// Test sorting of a container of array indexes to objects as opposed to a container of objects themselves.

		vector<TestObject>   toArray;
		vector<eastl_size_t> toiArray;

		for(eastl_size_t i = 0; i < 32; i++)
		{
			toArray.push_back(TestObject((int)rng.RandLimit(20)));
			toiArray.push_back(i);
		}

		quick_sort(toiArray.begin(), toiArray.end(), TestObjectIndexCompare(&toArray));
		EATEST_VERIFY(is_sorted(toiArray.begin(), toiArray.end(), TestObjectIndexCompare(&toArray)));
	}


	{
		// Test of special floating point sort in the presence of NaNs.
		vector<float> floatArray;
		union FloatInt32{ float f; int32_t i; } fi;

		for(int i = 0; i < 1000; i++)
		{
			fi.i = (int32_t)rng.Rand();
			floatArray.push_back(fi.f);
		}

		// Without SafeFloatCompare, the following quick_sort will crash, hang, or generate inconsistent results. 
		quick_sort(floatArray.begin(), floatArray.end(), SafeFloatCompare());
		EATEST_VERIFY(is_sorted(floatArray.begin(), floatArray.end(), SafeFloatCompare()));
	}

	{
		auto test_stable_sort = [&](auto testArray, size_t count)
		{
			auto isEven = [](auto val) { return (val % 2) == 0; };
			auto isOdd  = [](auto val) { return (val % 2) != 0; };

			for (size_t i = 0; i < count; i++)
				testArray.push_back((uint16_t)rng.Rand());

			vector<uint16_t> evenArray;
			vector<uint16_t> oddArray;

			eastl::copy_if(testArray.begin(), testArray.end(), eastl::back_inserter(evenArray), isEven);
			eastl::copy_if(testArray.begin(), testArray.end(), eastl::back_inserter(oddArray), isOdd);

			const auto boundary = eastl::stable_partition(testArray.begin(), testArray.end(), isEven);

			const auto evenCount = eastl::distance(testArray.begin(), boundary);
			const auto oddCount = eastl::distance(boundary, testArray.end());

			const auto evenExpectedCount = (ptrdiff_t)evenArray.size();
			const auto oddExpectedCount = (ptrdiff_t)oddArray.size();

			EATEST_VERIFY(evenCount == evenExpectedCount);
			EATEST_VERIFY(oddCount == oddExpectedCount);
			EATEST_VERIFY(eastl::equal(testArray.begin(), boundary, evenArray.begin()));
			EATEST_VERIFY(eastl::equal(boundary, testArray.end(), oddArray.begin()));
		};

		test_stable_sort(vector<uint16_t>(), 1000); // Test stable_partition
		test_stable_sort(vector<uint16_t>(), 0);	// Test stable_partition on empty container
		test_stable_sort(vector<uint16_t>(), 1);	// Test stable_partition on container of one element
		test_stable_sort(vector<uint16_t>(), 2);	// Test stable_partition on container of two element
		test_stable_sort(list<uint16_t>(),   0);	// Test stable_partition on bidirectional iterator (not random access)
	}

	// comb_sort ends with a comparator that reports every pair of distinct elements out of order (it then makes at
	// most one pass per element with a gap of 1). The comparator stops reporting that after a million calls, so that a
	// version that doesn't end still returns here. Where EASTL validates comparators, the first such answer asserts.
	#if !EASTL_VALIDATE_COMPARE_ENABLED
	{
		static int sCallCount;
		sCallCount = 0;
		auto notEqual = [](int a, int b) { return (++sCallCount <= 1000000) && (a != b); };
		vector<int> intArray;
		for(int i = 0; i < 100; i++)
			intArray.push_back((i * 37) % 100);
		comb_sort(intArray.begin(), intArray.end(), notEqual);
		EATEST_VERIFY(sCallCount < 20000);
		EATEST_VERIFY(intArray.size() == 100);
	}
	#endif

	// The stable sorts, radix_sort and stable_partition move elements between the range and their buffer instead of
	// copying them, so they also work on move-only elements, and stay stable.
	{
		struct KeyOf { typedef uint16_t radix_type; uint16_t operator()(const TestObject& t) const { return (uint16_t)t.mX; } };
		auto byKey = [](const TestObject& a, const TestObject& b) { return a.mX < b.mX; };
		auto isEven = [](const TestObject& t) { return (t.mX % 2) == 0; };
		const int count = 300;

		TestObject::Reset();
		{
			vector<TestObject> a, buffer(count);
			a.reserve(count);
			for(int i = 0; i < count; i++)
				a.push_back(TestObject((i * 7919) % 97));
			const int64_t copiesBefore = TestObject::sTOCopyCtorCount + TestObject::sTOCopyAssignCount;

			stable_sort(a.begin(), a.end(), byKey);
			EATEST_VERIFY(is_sorted(a.begin(), a.end(), byKey));
			reverse(a.begin(), a.end());
			merge_sort_buffer(a.begin(), a.end(), buffer.data(), byKey);
			EATEST_VERIFY(is_sorted(a.begin(), a.end(), byKey));
			reverse(a.begin(), a.end());
			tim_sort_buffer(a.begin(), a.end(), buffer.data(), byKey);
			EATEST_VERIFY(is_sorted(a.begin(), a.end(), byKey));
			reverse(a.begin(), a.end());
			radix_sort<TestObject*, KeyOf>(a.begin(), a.end(), buffer.begin());
			EATEST_VERIFY(is_sorted(a.begin(), a.end(), byKey));
			stable_partition(a.begin(), a.end(), isEven);
			EATEST_VERIFY(is_partitioned(a.begin(), a.end(), isEven));
			EATEST_VERIFY(TestObject::sTOCopyCtorCount + TestObject::sTOCopyAssignCount == copiesBefore);
		}
		EATEST_VERIFY(TestObject::IsClear());
		TestObject::Reset();

		// Move-only elements, with an index to check stability.
		auto byVal = [](const MoveOnlyTypeDefaultCtor& a, const MoveOnlyTypeDefaultCtor& b) { return (a.mVal / 1000) < (b.mVal / 1000); };
		vector<MoveOnlyTypeDefaultCtor> moveOnly, moveOnlyBuffer(count);
		for(int i = 0; i < count; i++)
			moveOnly.push_back(MoveOnlyTypeDefaultCtor(((i * 7919) % 97) * 1000 + i));
		stable_sort(moveOnly.begin(), moveOnly.end(), byVal);
		for(int i = 1; i < count; i++)
			EATEST_VERIFY(moveOnly[i - 1].mVal < moveOnly[i].mVal); // Equal keys keep their index order.
		tim_sort_buffer(moveOnly.begin(), moveOnly.end(), moveOnlyBuffer.data(), byVal);
		for(int i = 1; i < count; i++)
			EATEST_VERIFY(moveOnly[i - 1].mVal < moveOnly[i].mVal);
		stable_partition(moveOnly.begin(), moveOnly.end(), [](const MoveOnlyTypeDefaultCtor& m) { return (m.mVal / 1000) % 2 != 0; });
		for(int i = 1; i < count; i++)
			EATEST_VERIFY(((moveOnly[i - 1].mVal / 1000) % 2 == (moveOnly[i].mVal / 1000) % 2) ? (moveOnly[i - 1].mVal < moveOnly[i].mVal) : ((moveOnly[i - 1].mVal / 1000) % 2 != 0));
	}

	// sort and nth_element pick the pivot by iterator and partition around it in place: they only move and swap
	// elements, never copy them, so they also work on move-only elements, including tuple_vector's.
	{
		// A move-only type whose move operations are defaulted is trivially copyable, but can't be copied.
		struct DefaultedMoveOnly
		{
			int mKey;
			explicit DefaultedMoveOnly(int key) : mKey(key) {}
			DefaultedMoveOnly(DefaultedMoveOnly&&) = default;
			DefaultedMoveOnly& operator=(DefaultedMoveOnly&&) = default;
		};
		static_assert(eastl::is_trivially_copyable<DefaultedMoveOnly>::value && !eastl::is_copy_constructible<DefaultedMoveOnly>::value, "");
		auto keyLess = [](const DefaultedMoveOnly& a, const DefaultedMoveOnly& b) { return a.mKey < b.mKey; };

		vector<DefaultedMoveOnly> moveOnly;
		for(int i = 0; i < 500; i++)
			moveOnly.push_back(DefaultedMoveOnly((i * 7919) % 97));
		sort(moveOnly.begin(), moveOnly.end(), keyLess);
		EATEST_VERIFY(is_sorted(moveOnly.begin(), moveOnly.end(), keyLess));
		nth_element(moveOnly.begin(), moveOnly.begin() + 100, moveOnly.end(), [](const DefaultedMoveOnly& a, const DefaultedMoveOnly& b) { return b.mKey < a.mKey; });
		vector<int> descending;
		for(int i = 0; i < 500; i++)
			descending.push_back((i * 7919) % 97);
		sort(descending.begin(), descending.end(), greater<int>());
		EATEST_VERIFY(moveOnly[100].mKey == descending[100]);
	}
	{
		vector<int> keys;
		for(int i = 0; i < 500; i++)
			keys.push_back((i * 7919) % 97); // Many duplicates.
		sort(keys.begin(), keys.end());

		TestObject::Reset();
		{
			vector<TestObject> objects;
			for(int i = 0; i < 500; i++)
				objects.push_back(TestObject((i * 7919) % 97));
			const int64_t copiesBefore = TestObject::sTOCopyCtorCount + TestObject::sTOCopyAssignCount;

			sort(objects.begin(), objects.end());
			EATEST_VERIFY(is_sorted(objects.begin(), objects.end()));
			nth_element(objects.begin(), objects.begin() + 250, objects.end(), [](const TestObject& a, const TestObject& b) { return b < a; });
			nth_element(objects.begin(), objects.begin() + 100, objects.end());
			EATEST_VERIFY(objects[100].mX == keys[100]);
			EATEST_VERIFY(TestObject::sTOCopyCtorCount + TestObject::sTOCopyAssignCount == copiesBefore);
		}
		EATEST_VERIFY(TestObject::IsClear());
		TestObject::Reset();

		tuple_vector<int, MoveOnlyType> moveOnly;
		for(int i = 0; i < 500; i++)
			moveOnly.push_back(int((i * 7919) % 97), MoveOnlyType(i));
		auto lessKey = [](const auto& a, const auto& b) { return get<0>(a) < get<0>(b); };
		sort(moveOnly.begin(), moveOnly.end(), lessKey);
		EATEST_VERIFY(is_sorted(moveOnly.begin(), moveOnly.end(), lessKey));
		for(eastl_size_t i = 0; i < moveOnly.size(); i++)
			EATEST_VERIFY((get<1>(moveOnly[i]).mVal * 7919) % 97 == get<0>(moveOnly[i]));
		nth_element(moveOnly.begin(), moveOnly.begin() + 100, moveOnly.end(), lessKey);
		EATEST_VERIFY(get<0>(moveOnly[100]) == keys[100]);
	}

	// stable_sort (merge_sort) needs a buffer for only half the range, and none for a sorted range or one short enough to
	// be insertion-sorted. The buffer elements are moved in from the range, so the elements needn't be
	// default-constructible. The sizes go around the insertion sort lengths (32 elements when trivially copyable, else
	// 24) and their multiples.
	{
		struct Item { int mKey; int mIndex; }; // Trivially copyable.
		auto itemLess = [](const Item& a, const Item& b) { return a.mKey < b.mKey; };
		auto objectLess = [](const TestObject& a, const TestObject& b) { return (a.mX / 10000) < (b.mX / 10000); };
		auto keyOf = [](int order, int size, int i)
		{
			switch(order)
			{
				case 0:  return (i * 7919) % 97;                       // Scattered, with duplicates.
				case 1:  return i;                                     // Sorted.
				case 2:  return size - i;                              // Reversed.
				case 3:  return i % 30;                                // Sawtooth.
				case 4:  return ((i % 50) == 7) ? ((i * 7919) % 97) : i; // Nearly sorted.
				default: return (i < (size / 2)) ? i : (size - i);     // Organ pipe.
			}
		};

		const int sizes[] = { 0, 1, 2, 3, 23, 24, 25, 31, 32, 33, 47, 48, 49, 63, 64, 65, 96, 97, 191, 192, 193, 500, 1000, 1025 };
		for(int size : sizes)
		{
			for(int order = 0; order < 6; order++)
			{
				vector<Item> items;
				vector<TestObject> objects;
				for(int i = 0; i < size; i++)
				{
					items.push_back(Item{keyOf(order, size, i), i});
					objects.push_back(TestObject(keyOf(order, size, i) * 10000 + i));
				}

				stable_sort(items.begin(), items.end(), itemLess);
				stable_sort(objects.begin(), objects.end(), objectLess);

				// Each element is there once, with its key, and in order of key, then of original position.
				bool itemsOk = true, objectsOk = true;
				for(int i = 0; i < size; i++)
				{
					const int index = objects[i].mX % 10000;
					itemsOk = itemsOk && (items[i].mIndex >= 0) && (items[i].mIndex < size) && (items[i].mKey == keyOf(order, size, items[i].mIndex));
					objectsOk = objectsOk && (index < size) && ((objects[i].mX / 10000) == keyOf(order, size, index));
					if(i > 0)
					{
						itemsOk = itemsOk && ((items[i - 1].mKey < items[i].mKey) || ((items[i - 1].mKey == items[i].mKey) && (items[i - 1].mIndex < items[i].mIndex)));
						objectsOk = objectsOk && (objects[i - 1].mX < objects[i].mX);
					}
				}
				EATEST_VERIFY(itemsOk);
				EATEST_VERIFY(objectsOk);
			}
		}
		EATEST_VERIFY(TestObject::IsClear());
		TestObject::Reset();

		CountingAllocator::resetCount();
		{
			CountingAllocator allocator;
			vector<Item> items;
			for(int i = 0; i < 1001; i++)
				items.push_back(Item{(i * 7919) % 97, i});

			stable_sort(items.begin(), items.end(), allocator, itemLess);
			EATEST_VERIFY(CountingAllocator::getTotalAllocationCount() == 1);
			EATEST_VERIFY(CountingAllocator::getTotalAllocationSize() == 501 * sizeof(Item));
			EATEST_VERIFY(CountingAllocator::getActiveAllocationSize() == 0);

			stable_sort(items.begin(), items.end(), allocator, itemLess); // Already sorted.
			reverse(items.begin(), items.begin() + 32);
			stable_sort(items.begin(), items.begin() + 32, allocator, itemLess); // Insertion-sorted.
			EATEST_VERIFY(CountingAllocator::getTotalAllocationCount() == 1);
		}
		CountingAllocator::resetCount();

		vector<MoveOnlyType> moveOnly; // No default constructor.
		for(int i = 0; i < 300; i++)
			moveOnly.push_back(MoveOnlyType(((i * 7919) % 97) * 1000 + i));
		auto byVal = [](const MoveOnlyType& a, const MoveOnlyType& b) { return (a.mVal / 1000) < (b.mVal / 1000); };
		stable_sort(moveOnly.begin(), moveOnly.end(), byVal);
		for(int i = 1; i < 300; i++)
			EATEST_VERIFY(moveOnly[i - 1].mVal < moveOnly[i].mVal);
		reverse(moveOnly.begin(), moveOnly.end());
		merge_sort(moveOnly.begin(), moveOnly.end(), *get_default_allocator((EASTLAllocatorType*)NULL), byVal);
		for(int i = 1; i < 300; i++)
			EATEST_VERIFY((moveOnly[i - 1].mVal / 1000) <= (moveOnly[i].mVal / 1000));
	}

	// stable_sort and merge_sort_buffer merge arithmetic elements compared with less or greater without branches where
	// the runs interleave unpredictably, as random ones do. Doubles show whether that stays stable: -0.0 and 0.0 are
	// equivalent, so the signs of the zeros must stay in their original order. Zero is a middle key, so that ties between
	// zeros reach both ends of the merges, and random keys of 17 or 65 values make runs that interleave unpredictably
	// and still tie often.
	{
		auto keyOf = [&rng](int order, int size, int i)
		{
			const eastl_ssize_t half = size / 2;

			switch(order)
			{
				case 0:  return double(((i * 7919) % 97) - 48);        // Scattered, with duplicates.
				case 1:  return double(rng.RandRange(-8, 9));           // Random, 17 values.
				case 2:  return double(rng.RandRange(-32, 33));         // Random, 65 values.
				case 3:  return double(rng.RandRange(-half, half + 1)); // Random.
				case 4:  return double((i % 30) - 15);                  // Sawtooth.
				default: return double(half - i);                       // Reversed.
			}
		};
		auto zeroSigns = [](const vector<double>& values)
		{
			vector<int> signs;
			for(double value : values)
			{
				if(value == 0.0)
					signs.push_back(std::signbit(value) ? 1 : 0);
			}
			return signs;
		};

		const int sizes[] = { 33, 64, 65, 100, 257, 1000, 4097 };
		for(int size : sizes)
		{
			for(int order = 0; order < 6; order++)
			{
				vector<double> values;
				for(int i = 0; i < size; i++)
				{
					const double key = keyOf(order, size, i);
					values.push_back(((key == 0.0) && ((i % 3) == 1)) ? -0.0 : key);
				}

				vector<double> a(values), b(values), c(values), buffer(values.size());
				stable_sort(a.begin(), a.end());
				stable_sort(b.begin(), b.end(), greater<double>());
				merge_sort_buffer(c.begin(), c.end(), buffer.data(), less<double>());

				EATEST_VERIFY(is_sorted(a.begin(), a.end()) && (zeroSigns(a) == zeroSigns(values)));
				EATEST_VERIFY(is_sorted(b.begin(), b.end(), greater<double>()) && (zeroSigns(b) == zeroSigns(values)));
				EATEST_VERIFY(is_sorted(c.begin(), c.end()) && (zeroSigns(c) == zeroSigns(values)));

				sort(values.begin(), values.end());
				EATEST_VERIFY((a == values) && (c == values)); // Equal as doubles: -0.0 == 0.0.
			}
		}
	}

	// merge_sort_buffer, tim_sort_buffer and insertion_sort: each element is there once, in order of key, then of original
	// position, for sizes around the insertion sort lengths and tim_sort's minimum run, and several input orders.
	{
		struct Item { int mKey; int mIndex; };
		auto itemLess = [](const Item& a, const Item& b) { return a.mKey < b.mKey; };
		auto keyOf = [](int order, int size, int i)
		{
			switch(order)
			{
				case 0:  return (i * 7919) % 97;                       // Scattered, with duplicates.
				case 1:  return i;                                     // Sorted.
				case 2:  return size - i;                              // Reversed.
				case 3:  return i % 30;                                // Sawtooth.
				case 4:  return ((i % 50) == 7) ? ((i * 7919) % 97) : i; // Nearly sorted.
				default: return (i < (size / 2)) ? i : (size - i);     // Organ pipe.
			}
		};
		auto isStablySorted = [&](const vector<Item>& items, int order, int size)
		{
			bool ok = ((int)items.size() == size);
			for(int i = 0; ok && (i < size); i++)
			{
				ok = (items[i].mIndex >= 0) && (items[i].mIndex < size) && (items[i].mKey == keyOf(order, size, items[i].mIndex));
				if(ok && (i > 0))
					ok = (items[i - 1].mKey < items[i].mKey) || ((items[i - 1].mKey == items[i].mKey) && (items[i - 1].mIndex < items[i].mIndex));
			}
			return ok;
		};

		const int sizes[] = { 0, 1, 2, 3, 23, 24, 25, 31, 32, 33, 63, 64, 65, 127, 128, 129, 200, 1000, 1025 };
		for(int size : sizes)
		{
			for(int order = 0; order < 6; order++)
			{
				vector<Item> items, buffer(size);
				for(int i = 0; i < size; i++)
					items.push_back(Item{keyOf(order, size, i), i});

				vector<Item> a(items);
				merge_sort_buffer(a.begin(), a.end(), buffer.data(), itemLess);
				EATEST_VERIFY(isStablySorted(a, order, size));

				vector<Item> b(items);
				tim_sort_buffer(b.begin(), b.end(), buffer.data(), itemLess);
				EATEST_VERIFY(isStablySorted(b, order, size));

				if(size <= 200)
				{
					vector<Item> c(items);
					insertion_sort(c.begin(), c.end(), itemLess);
					EATEST_VERIFY(isStablySorted(c, order, size));
				}
			}
		}

		// insertion_sort moves no element that is already in place.
		TestObject::Reset();
		{
			vector<TestObject> objects;
			for(int i = 0; i < 100; i++)
				objects.push_back(TestObject(i));
			const int64_t movesBefore = TestObject::sTOMoveCtorCount + TestObject::sTOMoveAssignCount;
			insertion_sort(objects.begin(), objects.end());
			EATEST_VERIFY(TestObject::sTOMoveCtorCount + TestObject::sTOMoveAssignCount == movesBefore);
			EATEST_VERIFY(is_sorted(objects.begin(), objects.end()));
		}
		EATEST_VERIFY(TestObject::IsClear());
		TestObject::Reset();
	}

	// nth_element is O(n log n) in the worst case. McIlroy's adversary ("A Killer Adversary for Quicksort") is a valid
	// comparator that decides the order of the elements while the algorithm compares them, so as to defeat the median of
	// three: it made nth_element quadratic.
	{
		struct Adversary
		{
			int* mpValue; int mGas; int* mpSolidCount; int* mpCandidate; int* mpCallCount;

			bool operator()(int x, int y) const
			{
				++*mpCallCount;
				if((mpValue[x] == mGas) && (mpValue[y] == mGas))
					mpValue[(x == *mpCandidate) ? x : y] = (*mpSolidCount)++;
				if(mpValue[x] == mGas)
					*mpCandidate = x;
				else if(mpValue[y] == mGas)
					*mpCandidate = y;
				return mpValue[x] < mpValue[y];
			}
		};

		const int n = 3000;
		vector<int> ids, values(n, n);
		for(int i = 0; i < n; i++)
			ids.push_back(i);
		int solidCount = 0, candidate = 0, callCount = 0;
		nth_element(ids.begin(), ids.begin() + n / 2, ids.end(), Adversary{values.data(), n, &solidCount, &candidate, &callCount});
		EATEST_VERIFY(callCount < 20 * n * 12);
		for(int i = 0; i < n; i++)
			EATEST_VERIFY((i < n / 2) ? (values[ids[i]] <= values[ids[n / 2]]) : (values[ids[i]] >= values[ids[n / 2]]));
	}

	// sort (quick_sort) and nth_element on many sizes and input orders: ints compared with less (partitioned without
	// branches), ints with another comparator, and TestObjects (the pivot stays in place, nothing is copied). Sorted and
	// reversed ranges and ranges with few distinct values take linear time, and nth_element near an end of a sorted or
	// reversed range takes few comparisons.
	{
		auto keyOf = [](int order, int size, int i)
		{
			switch(order)
			{
				case 0:  return (i * 7919) % 97;                   // Scattered, with duplicates.
				case 1:  return i;                                 // Sorted.
				case 2:  return size - i;                          // Reversed.
				case 3:  return i % 7;                             // Sawtooth.
				case 4:  return (i < (size / 2)) ? i : (size - i); // Organ pipe.
				case 5:  return 3;                                 // All equal.
				default: return (i * 31) % 3;                      // Three values.
			}
		};
		auto intLess = [](int a, int b) { return a < b; };

		const int sizes[] = { 0, 1, 2, 3, 27, 28, 29, 30, 127, 128, 129, 130, 1000, 1025 };
		for(int size : sizes)
		{
			for(int order = 0; order < 7; order++)
			{
				vector<int> keys;
				for(int i = 0; i < size; i++)
					keys.push_back(keyOf(order, size, i));
				vector<int> sorted(keys);
				insertion_sort(sorted.begin(), sorted.end());

				vector<int> a(keys), b(keys);
				sort(a.begin(), a.end());
				sort(b.begin(), b.end(), intLess);
				EATEST_VERIFY(a == sorted);
				EATEST_VERIFY(b == sorted);

				TestObject::Reset();
				{
					vector<TestObject> objects;
					for(int i = 0; i < size; i++)
						objects.push_back(TestObject(keyOf(order, size, i)));
					const int64_t copiesBefore = TestObject::sTOCopyCtorCount + TestObject::sTOCopyAssignCount;

					sort(objects.begin(), objects.end());

					bool ok = true;
					for(int i = 0; i < size; i++)
						ok = ok && (objects[i].mX == sorted[i]);
					EATEST_VERIFY(ok);
					EATEST_VERIFY(TestObject::sTOCopyCtorCount + TestObject::sTOCopyAssignCount == copiesBefore);
				}
				EATEST_VERIFY(TestObject::IsClear());

				if(size > 0)
				{
					const int positions[] = { 0, size / 20, size / 3, size - 1 - size / 20, size - 1 };
					for(int nth : positions)
					{
						vector<int> c(keys), d(keys);
						nth_element(c.begin(), c.begin() + nth, c.end());
						nth_element(d.begin(), d.begin() + nth, d.end(), intLess);
						EATEST_VERIFY((c[nth] == sorted[nth]) && (d[nth] == sorted[nth]));

						bool ok = true;
						for(int i = 0; i < size; i++)
							ok = ok && ((i < nth) ? ((c[i] <= c[nth]) && (d[i] <= d[nth])) : ((c[i] >= c[nth]) && (d[i] >= d[nth])));
						EATEST_VERIFY(ok);
					}
				}
			}
		}
		TestObject::Reset();

		int callCount = 0;
		auto countingLess = [&callCount](int a, int b) { ++callCount; return a < b; };
		const int n = 10000;
		vector<int> v;
		for(int i = 0; i < n; i++)
			v.push_back(i);

		sort(v.begin(), v.end(), countingLess);
		EATEST_VERIFY(callCount < 3 * n);

		reverse(v.begin(), v.end());
		callCount = 0;
		sort(v.begin(), v.end(), countingLess);
		EATEST_VERIFY(callCount < 4 * n);

		for(int i = 0; i < n; i++)
			v[i] = (i * 7919) % 8;
		callCount = 0;
		sort(v.begin(), v.end(), countingLess);
		EATEST_VERIFY(callCount < 6 * n);
		EATEST_VERIFY(is_sorted(v.begin(), v.end()));

		// Pivots at the median would leave nth near an end of each part kept, which would then only halve. Where EASTL
		// validates comparators, the validations make comparisons too, so the counts aren't checked there.
		for(int i = 0; i < n; i++)
			v[i] = i;
		callCount = 0;
		nth_element(v.begin(), v.begin() + n / 100, v.end(), countingLess);
		EATEST_VERIFY(v[n / 100] == n / 100);
		#if !EASTL_VALIDATE_COMPARE_ENABLED
			EATEST_VERIFY(callCount < (3 * n / 2));
		#endif

		reverse(v.begin(), v.end());
		callCount = 0;
		nth_element(v.begin(), v.end() - 1 - n / 100, v.end(), countingLess);
		EATEST_VERIFY(v[n - 1 - n / 100] == n - 1 - n / 100);
		#if !EASTL_VALIDATE_COMPARE_ENABLED
			EATEST_VERIFY(callCount < (3 * n / 2));
		#endif
	}

	#if 0 // Disabled because it takes a long time and thus far seems to show no bug in quick_sort.
	{
		// Regression of Coverity report for Madden 2014 that quick_sort is reading beyond an array bounds within insertion_sort_simple.
		// The Madden code was sorting the 11 players on the field for a team by some criteria. We write 
		vector<int> intArray(11);
		for(eastl_size_t i = 0; i < intArray.size(); i++)
			intArray[i] = i;
		
		do {
			vector<int> intArrayCopy(intArray);

			// We need to verify that intArray[12] is never accessed. We could do that with a stomp allocator, 
			// which we don't currently have set up for the EASTL unit tests, or we could put a breakpoint in 
			// the debugger. Until we get a stomp allocator installed, do the breakpoint solution.
			quick_sort(intArrayCopy.begin(), intArrayCopy.end());
		} while(next_permutation(intArray.begin(), intArray.end()));
	}
	#endif

	EATEST_VERIFY(TestObject::IsClear());
	TestObject::Reset();

	return nErrorCount;
}









