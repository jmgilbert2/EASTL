/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#if defined(_MSC_VER)
	//#pragma warning(disable: 4267)  // 'argument' : conversion from 'size_t' to 'uint32_t', possible loss of data.
#endif


#include "EASTLTest.h"
#include <EASTL/numeric_limits.h>
#include <EASTL/set.h>
#include <EASTL/random.h>

EA_DISABLE_ALL_VC_WARNINGS()
#ifndef EA_COMPILER_NO_STANDARD_CPP_LIBRARY
	#include <random>
#endif
EA_RESTORE_ALL_VC_WARNINGS()


struct GeneratorUint8
{
	uint8_t mValue;
	GeneratorUint8() : mValue(0) {}
	uint8_t operator()(){ return mValue++; } // This is a pretty bad random number generator, but works for our tests.
};

struct GeneratorUint16
{
	uint16_t mValue;
	GeneratorUint16() : mValue(0) {}
	uint16_t operator()(){ return mValue++; }
};

struct GeneratorUint32
{
	uint32_t mValue;
	GeneratorUint32() : mValue(0) {}
	uint32_t operator()(){ return mValue++; }
};


// Verifies that shuffle with the given uniform random bit generator gives permutations, with each
// order equally likely, and that its indexes above 2^32 (for ranges that large) are uniform too.
template <typename URNG>
static int TestShuffle(URNG&& urng)
{
	int nErrorCount = 0;

	int intArray[] = { 3, 2, 6, 5, 4, 1 };

	eastl::shuffle(intArray, intArray + 0, urng);
	eastl::shuffle(intArray, intArray + 1, urng);
	EATEST_VERIFY(VerifySequence(intArray, intArray + 6, int(), "shuffle", 3, 2, 6, 5, 4, 1, -1));

	eastl::vector<eastl_size_t> rngArray, sortedArray;

	for(eastl_size_t i = 0; i < 200; ++i)
		sortedArray.push_back(i);

	rngArray = sortedArray;
	eastl::shuffle(rngArray.begin(), rngArray.end(), urng);
	EATEST_VERIFY(rngArray != sortedArray);
	EATEST_VERIFY(eastl::is_permutation(rngArray.begin(), rngArray.end(), sortedArray.begin()));

	// Each of the 6 orders of { 0, 1, 2 } should come out about 1000 times in 6000 (standard deviation 29).
	int counts[9] = {}; // Indexed by values[0] * 3 + values[1]; 0, 4 and 8 can't happen.
	int values[3] = { 0, 1, 2 };

	for(int i = 0; i < 6000; ++i)
	{
		eastl::shuffle(values, values + 3, urng);
		++counts[values[0] * 3 + values[1]];
	}

	for(int i = 0; i < 9; ++i)
		EATEST_VERIFY((i % 4 == 0) ? (counts[i] == 0) : ((counts[i] > 800) && (counts[i] < 1200)));

	// An index in a range of more than 2^32 elements needs 64 random bits.
	const uint64_t n = (UINT64_C(1) << 40) + 3;
	uint64_t minIndex = n, maxIndex = 0;

	for(int i = 0; i < 64; ++i)
	{
		const uint64_t index = eastl::Internal::urng_index(urng, n);
		minIndex = (index < minIndex) ? index : minIndex;
		maxIndex = (index > maxIndex) ? index : maxIndex;
	}

	EATEST_VERIFY((maxIndex < n) && (maxIndex > n / 2) && (minIndex < n / 2));

	return nErrorCount;
}




///////////////////////////////////////////////////////////////////////////////
// TestRandom
//
int TestRandom()
{
	int nErrorCount = 0;

	{
		// template<class IntType = int>
		// class uniform_int_distribution

		// The C++11 Standard defines a number of formal Generators, such as std::mersenne_twister_engine, 
		// linear_congruential_engine, discard_block_engine, etc.

		using namespace eastl;

		{
			eastl::uniform_int_distribution<uint8_t> uid(1, 6);
			GeneratorUint8 g;
 
			for(uint32_t i = 0; i < UINT8_MAX; i += 1)
			{
				uint8_t value = uid(g);
				EATEST_VERIFY((value >= 1) && (value <= 6));
				// To do: Validate the randomness of the value.
			}

			eastl::uniform_int_distribution<uint8_t> uid2(1, 6);
			EATEST_VERIFY(uid == uid2);
		}

		{
			eastl::uniform_int_distribution<uint16_t> uid(1, 6);
			GeneratorUint16 g;
 
			for(uint32_t i = 0; i < (UINT16_MAX - (UINT16_MAX / 50)); i += (UINT16_MAX / 50))
			{
				uint16_t value = uid(g);
				EATEST_VERIFY((value >= 1) && (value <= 6));
				// To do: Validate the randomness of the value.
			}

			eastl::uniform_int_distribution<uint16_t> uid2(1, 6);
			EATEST_VERIFY(uid == uid2);
		}

		{
			eastl::uniform_int_distribution<uint32_t> uid(1, 6);
			GeneratorUint32 g;
 
			for(uint32_t i = 0; i < (UINT32_MAX - (UINT32_MAX / 500)); i += (UINT32_MAX / 500))
			{
				uint32_t value = uid(g);
				EATEST_VERIFY((value >= 1) && (value <= 6));
				// To do: Validate the randomness of the value.
			}

			eastl::uniform_int_distribution<uint32_t> uid2(1, 6);
			EATEST_VERIFY(uid == uid2);
			EATEST_VERIFY((uid.a() == 1) && (uid.b() == 6) && (uid.min() == 1) && (uid.max() == 6));
		}
	}



	/// Example usage:
	///     eastl_size_t Rand(eastl_size_t n) { return (eastl_size_t)(rand() % n); } // Note: The C rand function is poor and slow.
	///     pointer_to_unary_function<eastl_size_t, eastl_size_t> randInstance(Rand);
	///     random_shuffle(pArrayBegin, pArrayEnd, randInstance);
	///
	/// Example usage:
	///     struct Rand{ eastl_size_t operator()(eastl_size_t n) { return (eastl_size_t)(rand() % n); } }; // Note: The C rand function is poor and slow.
	///     Rand randInstance;
	///     random_shuffle(pArrayBegin, pArrayEnd, randInstance);


	{
		// void random_shuffle(RandomAccessIterator first, RandomAccessIterator last, RandomNumberGenerator& rng)
		using namespace eastl;

		EASTLTest_Rand rng(EA::UnitTest::GetRandSeed());
		int intArray[] = { 3, 2, 6, 5, 4, 1 };

		random_shuffle(intArray, intArray + 0, rng);
		EATEST_VERIFY(VerifySequence(intArray, intArray + 6, int(), "random_shuffle", 3, 2, 6, 5, 4, 1, -1));

		random_shuffle(intArray, intArray + (sizeof(intArray) / sizeof(intArray[0])), rng);
		bool changed = false;
		for(int i = 0; (i < 5) && !changed; i++)
		{
			changed = (intArray[0] != 3) || (intArray[1] != 2) || (intArray[2] != 6) || 
					  (intArray[3] != 5) || (intArray[4] != 4) || (intArray[5] != 1);
		}
		EATEST_VERIFY(changed);

		// Test of possible bug report by user John Chin.
		// The report is that shuffling an ordered array 0, 1, 2, 3, 4 ... results in duplicates, such as 5, 2, 2, 4 ...
		eastl::vector<eastl_size_t> rngArray;

		for(eastl_size_t i = 0; i < 200; ++i)
			rngArray.push_back(i);

		random_shuffle(rngArray.begin(), rngArray.end(), rng);
		EATEST_VERIFY(rngArray.validate());

		eastl::set<eastl_size_t> intSet;

		for(eastl_size_t s = 0, sEnd = rngArray.size(); s < sEnd; ++s)
			intSet.insert(rngArray[s]);

		// If the shuffled array is unique, then a set of its values should be the same size as the array.
		EATEST_VERIFY(intSet.size() == rngArray.size());
	}

	{
		// template <typename RandomAccessIterator, typename UniformRandomNumberGenerator>
		// void shuffle(RandomAccessIterator first, RandomAccessIterator last, UniformRandomNumberGenerator&& urng)

		// EASTL's generators have no min() and max() and return any value of their result type:
		// eastl_size_t (64 bits on 64-bit platforms) and 8 bits.
		nErrorCount += TestShuffle(EASTLTest_Rand(EA::UnitTest::GetRandSeed()));
		nErrorCount += TestShuffle(RandGenT<uint8_t>(EA::UnitTest::GetRandSeed()));

		#ifndef EA_COMPILER_NO_STANDARD_CPP_LIBRARY
			std::mt19937 rng32(EA::UnitTest::GetRandSeed());
			nErrorCount += TestShuffle(rng32);
			nErrorCount += TestShuffle(std::mt19937_64(EA::UnitTest::GetRandSeed()));
			nErrorCount += TestShuffle(std::minstd_rand(EA::UnitTest::GetRandSeed())); // Its range, [1, 2^31 - 2], isn't a power of two.

			int intArray[] = { 3, 2, 6, 5, 4, 1 };
			const int sortedArray[] = { 1, 2, 3, 4, 5, 6 };
			eastl::shuffle(intArray, intArray + 6, std::mt19937());
			EATEST_VERIFY(eastl::is_permutation(intArray, intArray + 6, sortedArray));
		#endif
	}


	return nErrorCount;
}












