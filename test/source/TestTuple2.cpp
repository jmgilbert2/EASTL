/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

// <EASTL/tuple.h> must stay the first include of this file: it includes <EASTL/compare.h> before
// <EASTL/type_traits.h>, so the pair and tuple comparisons below check that compare.h, which they
// use when three-way comparison is available, doesn't depend on headers included before it.
#include <EASTL/tuple.h>

#include "EASTLTest.h"

#if EASTL_TUPLE_ENABLED

namespace
{

struct ConstantTupleEmpty {};
struct ConstantTupleExplicit
{
	explicit constexpr ConstantTupleExplicit(int value) : mValue(value) {}
	int mValue;
};

constexpr eastl::tuple<int, short> constantDefaultTuple{};
static_assert(eastl::get<0>(constantDefaultTuple) == 0 && eastl::get<1>(constantDefaultTuple) == 0, "default construction value-initializes elements");

constexpr eastl::tuple<int> constantValueTuple(7);
static_assert(eastl::get<0>(constantValueTuple) == 7 && eastl::get<int>(constantValueTuple) == 7, "constant indexed and typed access");
static_assert(eastl::get<0>(eastl::move(constantValueTuple)) == 7 && eastl::get<int>(eastl::move(constantValueTuple)) == 7, "constant const-rvalue access");

constexpr eastl::tuple<long> constantConvertedTuple(constantValueTuple);
constexpr eastl::tuple<ConstantTupleExplicit> constantExplicitTuple(constantValueTuple);
static_assert(eastl::get<0>(constantConvertedTuple) == 7 && eastl::get<0>(constantExplicitTuple).mValue == 7, "constant converting construction");

constexpr auto constantMadeTuple = eastl::make_tuple(11, short(13));
static_assert(eastl::get<0>(constantMadeTuple) == 11 && eastl::get<1>(constantMadeTuple) == 13, "make_tuple is usable in constant expressions");

constexpr eastl::tuple<ConstantTupleEmpty> constantEmptyTuple(ConstantTupleEmpty{});
static_assert(&eastl::get<0>(constantEmptyTuple) == &eastl::get<ConstantTupleEmpty>(constantEmptyTuple), "constant empty-member access");
constexpr eastl::tuple<ConstantTupleEmpty, ConstantTupleEmpty> constantRepeatedEmptyTuple{};
static_assert(&eastl::get<0>(constantRepeatedEmptyTuple) != &eastl::get<1>(constantRepeatedEmptyTuple), "constant repeated-empty member access");

constexpr int constantTupleReferent = 17;
constexpr eastl::tuple<const int&> constantReferenceTuple(constantTupleReferent);
static_assert(&eastl::get<0>(constantReferenceTuple) == &constantTupleReferent, "constant reference construction preserves its referent");

constexpr bool ModifyConstantTupleReference()
{
	int value = 1;
	const eastl::tuple<int&> reference(value);
	eastl::get<0>(reference) = 3;
	eastl::get<int&>(reference) = 5;
	return value == 5;
}
static_assert(ModifyConstantTupleReference(), "constant reference access can modify a local referent");

} // namespace

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
