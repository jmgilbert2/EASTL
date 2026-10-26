/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

// <EASTL/tuple.h> must stay the first include of this file: it includes <EASTL/compare.h> before
// <EASTL/type_traits.h>, so the pair and tuple comparisons below check that compare.h, which they
// use when three-way comparison is available, doesn't depend on headers included before it.
#include <EASTL/tuple.h>

#include "EASTLTest.h"

#if EASTL_TUPLE_ENABLED

int TestTupleIncludedFirstComparisons()
{
	using namespace eastl;

	int nErrorCount = 0;

	{
		pair<int, int> lesserPair(1, 2);
		pair<int, int> greaterPair(1, 3);
		EATEST_VERIFY(lesserPair < greaterPair && !(greaterPair < lesserPair));
		EATEST_VERIFY(lesserPair <= greaterPair && !(greaterPair <= lesserPair));
		EATEST_VERIFY(greaterPair > lesserPair && !(lesserPair > greaterPair));
		EATEST_VERIFY(greaterPair >= lesserPair && !(lesserPair >= greaterPair));

		#if defined(EA_COMPILER_HAS_THREE_WAY_COMPARISON)
		EATEST_VERIFY((lesserPair <=> greaterPair) < 0);
		EATEST_VERIFY((greaterPair <=> lesserPair) > 0);
		EATEST_VERIFY((lesserPair <=> lesserPair) == 0);
		#endif
	}

	{
		tuple<int, int> lesserTuple(1, 2);
		tuple<int, int> greaterTuple(1, 3);
		EATEST_VERIFY(lesserTuple < greaterTuple && !(greaterTuple < lesserTuple));
		EATEST_VERIFY(lesserTuple <= greaterTuple && !(greaterTuple <= lesserTuple));
		EATEST_VERIFY(greaterTuple > lesserTuple && !(lesserTuple > greaterTuple));
		EATEST_VERIFY(greaterTuple >= lesserTuple && !(lesserTuple >= greaterTuple));

		#if defined(EA_COMPILER_HAS_THREE_WAY_COMPARISON)
		EATEST_VERIFY((lesserTuple <=> greaterTuple) < 0);
		EATEST_VERIFY((greaterTuple <=> lesserTuple) > 0);
		EATEST_VERIFY((lesserTuple <=> lesserTuple) == 0);
		#endif
	}

	return nErrorCount;
}

#endif  // EASTL_TUPLE_ENABLED
