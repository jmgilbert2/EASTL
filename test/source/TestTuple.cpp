/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "EASTLTest.h"

EA_DISABLE_VC_WARNING(4623 4625 4413 4510)

#include <EASTL/any.h>
#include <EASTL/tuple.h>
#include <EASTL/unique_ptr.h>

#if EASTL_TUPLE_ENABLED

// An explicit instantiation defines every non-template member, so the const swap must not break it for
// elements that can't be swapped as const.
template class eastl::tuple<int, float>;

// The exception specifications of tuple, get, apply and make_from_tuple follow the standard (the conditional ones as
// MSVC's STL strengthens them).
namespace NoexceptTupleTest
{
	// Everything noexcept, and constructible from an int.
	struct Nothrow
	{
		Nothrow() EA_NOEXCEPT; Nothrow(int) EA_NOEXCEPT; Nothrow(const Nothrow&) EA_NOEXCEPT; Nothrow(Nothrow&&) EA_NOEXCEPT;
		Nothrow& operator=(int) EA_NOEXCEPT; Nothrow& operator=(const Nothrow&) EA_NOEXCEPT; Nothrow& operator=(Nothrow&&) EA_NOEXCEPT;
	};

	// Everything may throw, and constructible from an int.
	struct Throwing
	{
		Throwing(); Throwing(int); Throwing(const Throwing&); Throwing(Throwing&&) EA_NOEXCEPT_IF(false);
		Throwing& operator=(int); Throwing& operator=(const Throwing&); Throwing& operator=(Throwing&&) EA_NOEXCEPT_IF(false);
	};

	struct NothrowFunctor { int operator()(int, int) const EA_NOEXCEPT; };
	struct ThrowingFunctor { int operator()(int, int) const; };
	struct NothrowMaker { NothrowMaker(int, int) EA_NOEXCEPT; };
	struct ThrowingMaker { ThrowingMaker(int, int); };

	typedef eastl::tuple<Nothrow, Nothrow>     NothrowTuple;
	typedef eastl::tuple<Throwing, Throwing>   ThrowingTuple;
	typedef eastl::tuple<Nothrow, Throwing>    MixedTuple;
	typedef eastl::tuple<int, int>             IntTuple;

	// Constructors.
	static_assert(noexcept(eastl::tuple<>()), "tuple<>() is noexcept");
	static_assert(noexcept(NothrowTuple()), "tuple() is noexcept when the elements' default constructors are");
	static_assert(!noexcept(ThrowingTuple()), "tuple() may throw when the elements' default constructors may");
	static_assert(!noexcept(MixedTuple()), "tuple() may throw when one element's default constructor may");
	static_assert(noexcept(NothrowTuple(eastl::declval<const Nothrow&>(), eastl::declval<const Nothrow&>())), "tuple(const T&...) is noexcept when the copies are");
	static_assert(!noexcept(ThrowingTuple(eastl::declval<const Throwing&>(), eastl::declval<const Throwing&>())), "tuple(const T&...) may throw when the copies may");
	static_assert(noexcept(NothrowTuple(eastl::declval<Nothrow>(), eastl::declval<Nothrow>())), "tuple(U&&...) is noexcept when the constructions are");
	static_assert(noexcept(NothrowTuple(1, 2)), "tuple(U&&...) is noexcept when the constructions from the arguments are");
	static_assert(!noexcept(ThrowingTuple(1, 2)), "tuple(U&&...) may throw when the constructions from the arguments may");
	static_assert(!noexcept(MixedTuple(1, 2)), "tuple(U&&...) may throw when one construction from the arguments may");
	static_assert(noexcept(NothrowTuple(eastl::declval<const IntTuple&>())), "tuple(const tuple<U...>&) is noexcept when the constructions are");
	static_assert(!noexcept(ThrowingTuple(eastl::declval<const IntTuple&>())), "tuple(const tuple<U...>&) may throw when the constructions may");
	static_assert(noexcept(NothrowTuple(eastl::declval<IntTuple>())), "tuple(tuple<U...>&&) is noexcept when the constructions are");
	static_assert(!noexcept(ThrowingTuple(eastl::declval<IntTuple>())), "tuple(tuple<U...>&&) may throw when the constructions may");
	static_assert(noexcept(NothrowTuple(eastl::declval<const eastl::pair<int, int>&>())), "tuple(const pair<U1, U2>&) is noexcept when the constructions are");
	static_assert(!noexcept(ThrowingTuple(eastl::declval<const eastl::pair<int, int>&>())), "tuple(const pair<U1, U2>&) may throw when the constructions may");

	// Assignment.
	static_assert(noexcept(eastl::declval<NothrowTuple&>() = eastl::declval<const NothrowTuple&>()), "tuple copy assignment is noexcept when the elements' are");
	static_assert(!noexcept(eastl::declval<ThrowingTuple&>() = eastl::declval<const ThrowingTuple&>()), "tuple copy assignment may throw when the elements' may");
	static_assert(noexcept(eastl::declval<NothrowTuple&>() = eastl::declval<const IntTuple&>()), "tuple::operator=(const tuple<U...>&) is noexcept when the assignments are");
	static_assert(!noexcept(eastl::declval<ThrowingTuple&>() = eastl::declval<const IntTuple&>()), "tuple::operator=(const tuple<U...>&) may throw when the assignments may");
	static_assert(noexcept(eastl::declval<NothrowTuple&>() = eastl::declval<IntTuple>()), "tuple::operator=(tuple<U...>&&) is noexcept when the assignments are");
	static_assert(!noexcept(eastl::declval<MixedTuple&>() = eastl::declval<IntTuple>()), "tuple::operator=(tuple<U...>&&) may throw when one assignment may");
	static_assert(noexcept(eastl::declval<NothrowTuple&>() = eastl::declval<const eastl::pair<int, int>&>()), "tuple::operator=(const pair<U1, U2>&) is noexcept when the assignments are");
	static_assert(!noexcept(eastl::declval<ThrowingTuple&>() = eastl::declval<const eastl::pair<int, int>&>()), "tuple::operator=(const pair<U1, U2>&) may throw when the assignments may");

	// swap.
	static_assert(noexcept(eastl::declval<eastl::tuple<>&>().swap(eastl::declval<eastl::tuple<>&>())), "tuple<>::swap is noexcept");
	static_assert(noexcept(eastl::declval<const eastl::tuple<>&>().swap(eastl::declval<const eastl::tuple<>&>())), "tuple<>::swap const is noexcept");
	static_assert(noexcept(eastl::declval<NothrowTuple&>().swap(eastl::declval<NothrowTuple&>())), "tuple::swap is noexcept when the elements' swaps are");
	static_assert(noexcept(eastl::swap(eastl::declval<NothrowTuple&>(), eastl::declval<NothrowTuple&>())), "swap(tuple&, tuple&) is noexcept when the elements' swaps are");
	static_assert(!noexcept(eastl::declval<ThrowingTuple&>().swap(eastl::declval<ThrowingTuple&>())), "tuple::swap may throw when the elements' swaps may");
	static_assert(!noexcept(eastl::swap(eastl::declval<ThrowingTuple&>(), eastl::declval<ThrowingTuple&>())), "swap(tuple&, tuple&) may throw when the elements' swaps may");
	static_assert(!noexcept(eastl::declval<MixedTuple&>().swap(eastl::declval<MixedTuple&>())), "tuple::swap may throw when one element's swap may");
	static_assert(noexcept(eastl::declval<const eastl::tuple<int&, int&>&>().swap(eastl::declval<const eastl::tuple<int&, int&>&>())), "tuple::swap const is noexcept for references to ints");
	static_assert(noexcept(eastl::swap(eastl::declval<const eastl::tuple<int&, int&>&>(), eastl::declval<const eastl::tuple<int&, int&>&>())), "swap(const tuple&, const tuple&) is noexcept for references to ints");

	// get.
	static_assert(noexcept(eastl::get<0>(eastl::declval<ThrowingTuple&>())), "get<I>(tuple&) is noexcept");
	static_assert(noexcept(eastl::get<1>(eastl::declval<const ThrowingTuple&>())), "get<I>(const tuple&) is noexcept");
	static_assert(noexcept(eastl::get<0>(eastl::declval<ThrowingTuple>())), "get<I>(tuple&&) is noexcept");
	static_assert(noexcept(eastl::get<1>(eastl::declval<const ThrowingTuple>())), "get<I>(const tuple&&) is noexcept");
	static_assert(noexcept(eastl::get<Nothrow>(eastl::declval<MixedTuple&>())), "get<T>(tuple&) is noexcept");
	static_assert(noexcept(eastl::get<Nothrow>(eastl::declval<const MixedTuple&>())), "get<T>(const tuple&) is noexcept");
	static_assert(noexcept(eastl::get<Nothrow>(eastl::declval<MixedTuple>())), "get<T>(tuple&&) is noexcept");
	static_assert(noexcept(eastl::get<Nothrow>(eastl::declval<const MixedTuple>())), "get<T>(const tuple&&) is noexcept");

	// apply and make_from_tuple.
	static_assert(noexcept(eastl::apply(NothrowFunctor(), eastl::declval<const IntTuple&>())), "apply is noexcept when the call is");
	static_assert(!noexcept(eastl::apply(ThrowingFunctor(), eastl::declval<const IntTuple&>())), "apply may throw when the call may");
	static_assert(noexcept(eastl::make_from_tuple<NothrowMaker>(eastl::declval<const IntTuple&>())), "make_from_tuple is noexcept when the construction is");
	static_assert(!noexcept(eastl::make_from_tuple<ThrowingMaker>(eastl::declval<const IntTuple&>())), "make_from_tuple may throw when the construction may");
}

namespace TestTupleInternal
{

struct DefaultConstructibleType
{
	static const int defaultVal = 0x1EE7C0DE;
	DefaultConstructibleType() : mVal(defaultVal) {}
	int mVal;
};

struct OperationCountingType
{
	OperationCountingType() : mVal() { ++mDefaultConstructorCalls; }
	OperationCountingType(int x) : mVal(x) { ++mIntConstructorCalls; }
	OperationCountingType(const OperationCountingType& x) : mVal(x.mVal) { ++mCopyConstructorCalls; }
	OperationCountingType(OperationCountingType&& x) : mVal(x.mVal)
	{
		++mMoveConstructorCalls;
		x.mVal = 0;
	}
	OperationCountingType& operator=(const OperationCountingType& x)
	{
		mVal = x.mVal;
		++mCopyAssignmentCalls;
		return *this;
	}
	OperationCountingType& operator=(OperationCountingType&& x)
	{
		mVal = x.mVal;
		x.mVal = 0;
		++mMoveAssignmentCalls;
		return *this;
	}
	~OperationCountingType() { ++mDestructorCalls; }

	int mVal;

	static void ResetCounters()
	{
		mDefaultConstructorCalls = 0;
		mIntConstructorCalls = 0;
		mCopyConstructorCalls = 0;
		mMoveConstructorCalls = 0;
		mCopyAssignmentCalls = 0;
		mMoveAssignmentCalls = 0;
		mDestructorCalls = 0;
	}

	static int mDefaultConstructorCalls;
	static int mIntConstructorCalls;
	static int mCopyConstructorCalls;
	static int mMoveConstructorCalls;
	static int mCopyAssignmentCalls;
	static int mMoveAssignmentCalls;
	static int mDestructorCalls;
};

int OperationCountingType::mDefaultConstructorCalls = 0;
int OperationCountingType::mIntConstructorCalls = 0;
int OperationCountingType::mCopyConstructorCalls = 0;
int OperationCountingType::mMoveConstructorCalls = 0;
int OperationCountingType::mCopyAssignmentCalls = 0;
int OperationCountingType::mMoveAssignmentCalls = 0;
int OperationCountingType::mDestructorCalls = 0;

// Constructible from anything, so a tuple<const FromAnything&> could bind its element to a temporary built from
// another tuple.
struct FromAnything
{
	FromAnything() = default;
	template <typename T> FromAnything(const T&) {}
};

// Not movable, but constructible from anything else, so a tuple of it could be "moved" by building the element from
// the tuple's internals.
struct DeletedMoveFromAnything
{
	DeletedMoveFromAnything() = default;
	DeletedMoveFromAnything(DeletedMoveFromAnything&&) = delete;
	template <typename T> DeletedMoveFromAnything(T&&) {}
};

struct DeletedMoveFromAnythingNonEmpty
{
	int mX = 0;
	DeletedMoveFromAnythingNonEmpty() = default;
	DeletedMoveFromAnythingNonEmpty(DeletedMoveFromAnythingNonEmpty&&) = delete;
	template <typename T> DeletedMoveFromAnythingNonEmpty(T&&) {}
};

}  // namespace TestTupleInternal

#if EASTL_INLINE_VARIABLE_ENABLED
// TestUtility.cpp checks that it sees the same ignore and piecewise_construct objects as this translation unit.
const void* TestTupleIgnoreAddress() { return &eastl::ignore; }
const void* TestTuplePiecewiseConstructAddress() { return &eastl::piecewise_construct; }
#endif

int TestTupleIncludedFirstComparisons();

int TestTuple()
{
	using namespace eastl;
	using namespace TestTupleInternal;

	int nErrorCount = 0;

	static_assert(tuple_size<tuple<int>>::value == 1, "tuple_size<tuple<T>> test failed.");
	static_assert(tuple_size<const tuple<int>>::value == 1, "tuple_size<const tuple<T>> test failed.");
	static_assert(tuple_size<const tuple<const int>>::value == 1, "tuple_size<const tuple<const T>> test failed.");
	static_assert(tuple_size<volatile tuple<int>>::value == 1, "tuple_size<volatile tuple<T>> test failed.");
	static_assert(tuple_size<const volatile tuple<int>>::value == 1, "tuple_size<const volatile tuple<T>> test failed.");
	static_assert(tuple_size<tuple<int, float, bool>>::value == 3, "tuple_size<tuple<T, T, T>> test failed.");

	static_assert(is_same<tuple_element_t<0, tuple<int>>, int>::value, "tuple_element<I, T> test failed.");
	static_assert(is_same<tuple_element_t<1, tuple<float, int>>, int>::value, "tuple_element<I, T> test failed.");
	static_assert(is_same<tuple_element_t<1, tuple<float, const int>>, const int>::value, "tuple_element<I, T> test failed.");
	static_assert(is_same<tuple_element_t<1, tuple<float, volatile int>>, volatile int>::value, "tuple_element<I, T> test failed.");
	static_assert(is_same<tuple_element_t<1, tuple<float, const volatile int>>, const volatile int>::value, "tuple_element<I, T> test failed.");
	static_assert(is_same<tuple_element_t<1, tuple<float, int&>>, int&>::value, "tuple_element<I, T> test failed.");

	{
		tuple<int> aSingleElementTuple(1);
		EATEST_VERIFY(get<0>(aSingleElementTuple) == 1);
		get<0>(aSingleElementTuple) = 2;
		EATEST_VERIFY(get<0>(aSingleElementTuple) == 2);
		get<int>(aSingleElementTuple) = 3;
		EATEST_VERIFY(get<int>(aSingleElementTuple) == 3);

		const tuple<int> aConstSingleElementTuple(3);
		EATEST_VERIFY(get<0>(aConstSingleElementTuple) == 3);
		EATEST_VERIFY(get<int>(aConstSingleElementTuple) == 3);

		tuple<DefaultConstructibleType> aDefaultConstructedTuple;
		EATEST_VERIFY(get<0>(aDefaultConstructedTuple).mVal == DefaultConstructibleType::defaultVal);

		OperationCountingType::ResetCounters();
		tuple<OperationCountingType> anOperationCountingTuple;
		EATEST_VERIFY(OperationCountingType::mDefaultConstructorCalls == 1 &&
					  get<0>(anOperationCountingTuple).mVal == 0);
		get<0>(anOperationCountingTuple).mVal = 1;
		tuple<OperationCountingType> anotherOperationCountingTuple(anOperationCountingTuple);
		EATEST_VERIFY(OperationCountingType::mDefaultConstructorCalls == 1 &&
					  OperationCountingType::mCopyConstructorCalls == 1 &&
					  get<0>(anotherOperationCountingTuple).mVal == 1);
		get<0>(anOperationCountingTuple).mVal = 2;
		anotherOperationCountingTuple = anOperationCountingTuple;
		EATEST_VERIFY(
			OperationCountingType::mDefaultConstructorCalls == 1 && OperationCountingType::mCopyConstructorCalls == 1 &&
			OperationCountingType::mCopyAssignmentCalls == 1 && get<0>(anotherOperationCountingTuple).mVal == 2);

		OperationCountingType::ResetCounters();
		tuple<OperationCountingType> yetAnotherOperationCountingTuple(OperationCountingType(5));
		EATEST_VERIFY(
			OperationCountingType::mMoveConstructorCalls == 1 && OperationCountingType::mDefaultConstructorCalls == 0 &&
			OperationCountingType::mCopyConstructorCalls == 0 && get<0>(yetAnotherOperationCountingTuple).mVal == 5);
	}

	EATEST_VERIFY(OperationCountingType::mDestructorCalls == 4);

	{
		// Test constructor
		tuple<int, float, bool> aTuple(1, 1.0f, true);
		EATEST_VERIFY(get<0>(aTuple) == 1);
		EATEST_VERIFY(get<1>(aTuple) == 1.0f);
		EATEST_VERIFY(get<2>(aTuple) == true);
		EATEST_VERIFY(get<int>(aTuple) == 1);
		EATEST_VERIFY(get<float>(aTuple) == 1.0f);
		EATEST_VERIFY(get<bool>(aTuple) == true);

		get<1>(aTuple) = 2.0f;
		EATEST_VERIFY(get<1>(aTuple) == 2.0f);

		// Test copy constructor
		tuple<int, float, bool> anotherTuple(aTuple);
		EATEST_VERIFY(get<0>(anotherTuple) == 1 && get<1>(anotherTuple) == 2.0f && get<2>(anotherTuple) == true);

		// Test copy assignment
		tuple<int, float, bool> yetAnotherTuple(2, 3.0f, true);
		EATEST_VERIFY(get<0>(yetAnotherTuple) == 2 && get<1>(yetAnotherTuple) == 3.0f &&
					  get<2>(yetAnotherTuple) == true);
		yetAnotherTuple = anotherTuple;
		EATEST_VERIFY(get<0>(yetAnotherTuple) == 1 && get<1>(yetAnotherTuple) == 2.0f &&
					  get<2>(yetAnotherTuple) == true);

		// Test converting 'copy' constructor (from a tuple of different type whose members are each convertible)
		tuple<double, double, bool> aDifferentTuple(aTuple);
		EATEST_VERIFY(get<0>(aDifferentTuple) == 1.0 && get<1>(aDifferentTuple) == 2.0 &&
					  get<2>(aDifferentTuple) == true);

		// Test converting assignment operator (from a tuple of different type whose members are each convertible)
		tuple<double, double, bool> anotherDifferentTuple;
		EATEST_VERIFY(get<0>(anotherDifferentTuple) == 0.0 && get<1>(anotherDifferentTuple) == 0.0 &&
					  get<2>(anotherDifferentTuple) == false);
		anotherDifferentTuple = anotherTuple;
		EATEST_VERIFY(get<0>(anotherDifferentTuple) == 1.0 && get<1>(anotherDifferentTuple) == 2.0 &&
					  get<2>(anotherDifferentTuple) == true);

		// Test default initialization (built in types should be value initialized rather than default initialized)
		tuple<int, float, bool> aDefaultInitializedTuple;
		EATEST_VERIFY(get<0>(aDefaultInitializedTuple) == 0 && get<1>(aDefaultInitializedTuple) == 0.0f &&
					  get<2>(aDefaultInitializedTuple) == false);
	}

	// The converting constructors from another tuple or a pair are explicit exactly when an element conversion isn't
	// implicit, like std::tuple's.
	{
		static_assert(is_convertible_v<tuple<int, int>, tuple<long, double>>, "tuple(tuple<U...>&&) is implicit");
		static_assert(is_convertible_v<const pair<int, int>&, tuple<long, double>>, "tuple(const pair<U1, U2>&) is implicit");
		static_assert(is_constructible_v<tuple<TestObject>, tuple<int>&>, "tuple(tuple<U...>&) is explicit");
		static_assert(!is_convertible_v<tuple<int>&, tuple<TestObject>>, "tuple(tuple<U...>&) is explicit");
		static_assert(is_constructible_v<tuple<TestObject, int>, pair<int, int>>, "tuple(pair<U1, U2>&&) is explicit");
		static_assert(!is_convertible_v<pair<int, int>, tuple<TestObject, int>>, "tuple(pair<U1, U2>&&) is explicit");

		tuple<int> intTuple(3);
		tuple<TestObject> objectTuple(intTuple);
		EATEST_VERIFY(get<0>(objectTuple).mX == 3);

		tuple<TestObject, int> objectIntTuple(make_pair(4, 5));
		EATEST_VERIFY((get<0>(objectIntTuple).mX == 4) && (get<1>(objectIntTuple) == 5));
	}

	// A one-element tuple<T> holds a whole source tuple when T can be built from it, like std::tuple, but copying or
	// moving a tuple<T> still copies or moves the element.
	{
		tuple<int> intTuple(5);
		tuple<any> holdsTuple(intTuple);
		EATEST_VERIFY(any_cast<tuple<int>>(&get<0>(holdsTuple)) != nullptr);
		tuple<any> holdsMovedTuple(eastl::move(intTuple));
		EATEST_VERIFY(any_cast<tuple<int>>(&get<0>(holdsMovedTuple)) != nullptr);

		tuple<any> holdsInt(5);
		tuple<any> copied(holdsInt);
		EATEST_VERIFY(any_cast<int>(&get<0>(copied)) != nullptr);
		const tuple<any>& constHoldsInt = holdsInt;
		tuple<any> copiedFromConst(constHoldsInt);
		EATEST_VERIFY(any_cast<int>(&get<0>(copiedFromConst)) != nullptr);
		tuple<any> moved(eastl::move(holdsInt));
		EATEST_VERIFY(any_cast<int>(&get<0>(moved)) != nullptr);
	}

	// tuple works in constant expressions, like std::tuple: construction, get, comparisons and tuple_cat since C++14,
	// assignment since C++20.
	{
		constexpr tuple<int, long> values(1, 2);
		static_assert((get<0>(values) == 1) && (get<long>(values) == 2), "constexpr construction and get");

		constexpr tuple<int, float> defaulted{};
		static_assert(get<0>(defaulted) == 0, "constexpr default construction");

		constexpr tuple<long, long> converted(values);
		constexpr tuple<int, long> fromPair(make_pair(3, 4));
		static_assert((get<1>(converted) == 2) && (get<1>(fromPair) == 4), "constexpr converting construction");
		static_assert((values == converted) && (values < fromPair) && !(fromPair <= values), "constexpr comparisons");

		constexpr auto concatenated = tuple_cat(values, make_tuple(5), fromPair);
		static_assert((get<2>(concatenated) == 5) && (get<4>(concatenated) == 4), "constexpr tuple_cat");
		static_assert(get<0>(tuple<int, int>(6, 7)) == 6, "constexpr get on an rvalue");

		static constexpr int seven = 7;
		constexpr tuple<const int&> reference(seven);
		static_assert(get<0>(reference) == 7, "constexpr reference element");

	#if defined(EA_COMPILER_CPP20_ENABLED)
		constexpr auto assign = []
		{
			tuple<int, int> a(1, 2);
			a = tuple<int, int>(3, 4);
			tuple<long, long> b;
			b = a;
			b = make_pair(5, 6);
			return get<0>(a) + static_cast<int>(get<1>(b));
		};
		static_assert(assign() == 9, "constexpr assignment");

		constexpr auto swapped = []
		{
			tuple<int, long> a(1, 2), b(3, 4);
			a.swap(b);
			swap(a, b);
			a.swap(b);
			tuple<> e1, e2;
			e1.swap(e2);
			return (get<0>(a) == 3) && (get<1>(a) == 4) && (get<0>(b) == 1) && (get<1>(b) == 2);
		};
		static_assert(swapped(), "constexpr swap");
	#endif
	}

	{
		// Test some other cases with typed-getter
		tuple<double, double, bool> aTupleWithRepeatedType(1.0f, 2.0f, true);
		EATEST_VERIFY(get<bool>(aTupleWithRepeatedType) == true);

		tuple<double, bool, double> anotherTupleWithRepeatedType(1.0f, true, 2.0f);
		EATEST_VERIFY(get<bool>(anotherTupleWithRepeatedType) == true);

		tuple<bool, double, double> yetAnotherTupleWithRepeatedType(true, 1.0f, 2.0f);
		EATEST_VERIFY(get<bool>(yetAnotherTupleWithRepeatedType) == true);

		struct floatOne { float val; };
		struct floatTwo { float val; };
		tuple<floatOne, floatTwo> aTupleOfStructs({ 1.0f }, { 2.0f } );
		EATEST_VERIFY(get<floatOne>(aTupleOfStructs).val == 1.0f);
		EATEST_VERIFY(get<floatTwo>(aTupleOfStructs).val == 2.0f);
		
		const tuple<double, double, bool> aConstTuple(aTupleWithRepeatedType);
		const bool& constRef = get<bool>(aConstTuple);
		EATEST_VERIFY(constRef == true);

		const bool&& constRval = get<bool>(eastl::move(aTupleWithRepeatedType));
		EATEST_VERIFY(constRval == true);
	}

	// Like std::get, get on a const tuple adds a top-level const to the element type, so a reference element stays a
	// non-const reference, and a const rvalue tuple gives const rvalue references.
	{
		int i = 1;
		const tuple<int&, int> constTuple(i, 2);
		static_assert(is_same_v<decltype(get<0>(constTuple)), int&>, "get<0>(const tuple<int&, int>&) is int&");
		static_assert(is_same_v<decltype(get<int&>(constTuple)), int&>, "get<int&>(const tuple<int&, int>&) is int&");
		get<0>(constTuple) = 3;
		EATEST_VERIFY(i == 3);

		static_assert(is_same_v<decltype(get<0>(eastl::move(constTuple))), int&>, "get<0>(const tuple<int&, int>&&) is int&");
		static_assert(is_same_v<decltype(get<1>(eastl::move(constTuple))), const int&&>, "get<1>(const tuple<int&, int>&&) is const int&&");
		static_assert(is_same_v<decltype(get<int>(eastl::move(constTuple))), const int&&>, "get<int>(const tuple<int&, int>&&) is const int&&");
		const int&& constRvalue = get<int>(eastl::move(constTuple));
		EATEST_VERIFY(&constRvalue == &get<1>(constTuple));
	}

	{
		tuple<int, float> aTupleWithDefaultInit(1, {});
		EATEST_VERIFY((get<0>(aTupleWithDefaultInit) == 1) && (get<1>(aTupleWithDefaultInit) == 0.0f));

		// tuple construction from pair
		pair<int, float> aPair(1, 2.0f);
		tuple<int, float> aTuple(aPair);
		EATEST_VERIFY(get<0>(aTuple) == 1 && get<1>(aTuple) == 2.0f);
		tuple<double, double> anotherTuple(aPair);
		EATEST_VERIFY(get<0>(anotherTuple) == 1.0 && get<1>(anotherTuple) == 2.0);
		anotherTuple = make_pair(2, 3);
		EATEST_VERIFY(get<0>(anotherTuple) == 2.0 && get<1>(anotherTuple) == 3.0);

		// operators: ==, !=, <
		anotherTuple = aTuple;
		EATEST_VERIFY(aTuple == anotherTuple);
		EATEST_VERIFY(!(aTuple < anotherTuple) && !(anotherTuple < aTuple));
		tuple<double, double> aDefaultInitTuple;
		EATEST_VERIFY(aTuple != aDefaultInitTuple);
		EATEST_VERIFY(aDefaultInitTuple < aTuple);

		#if defined(EA_COMPILER_HAS_THREE_WAY_COMPARISON)
		EATEST_VERIFY((aTuple <=> anotherTuple) == 0);
		EATEST_VERIFY((aTuple <=> anotherTuple) >= 0);
		EATEST_VERIFY((anotherTuple <=> aTuple) >= 0);
		EATEST_VERIFY((aTuple <=> aDefaultInitTuple) != 0);
		EATEST_VERIFY((aDefaultInitTuple <=> aTuple) < 0);
		#endif

		tuple<int, int, int> lesserTuple(1, 2, 3);
		tuple<int, int, int> greaterTuple(1, 2, 4);
		EATEST_VERIFY(lesserTuple < greaterTuple && !(greaterTuple < lesserTuple) && greaterTuple > lesserTuple &&
					  !(lesserTuple > greaterTuple));

		#if defined(EA_COMPILER_HAS_THREE_WAY_COMPARISON)
		EATEST_VERIFY((lesserTuple <=> greaterTuple) != 0);
		EATEST_VERIFY((lesserTuple <=> greaterTuple) < 0);
		EATEST_VERIFY((lesserTuple <=> greaterTuple) <= 0);
		EATEST_VERIFY((greaterTuple <=> lesserTuple) > 0);
		EATEST_VERIFY((greaterTuple <=> lesserTuple) >= 0);
		#endif

		tuple<int, float, TestObject> valTup(2, 2.0f, TestObject(2));
		tuple<int&, float&, TestObject&> refTup(valTup);
		tuple<const int&, const float&, const TestObject&> constRefTup(valTup);

		EATEST_VERIFY(get<0>(refTup) == get<0>(valTup));
		EATEST_VERIFY(get<1>(refTup) == get<1>(valTup));
		EATEST_VERIFY(refTup == valTup);
		EATEST_VERIFY(get<0>(refTup) == get<0>(constRefTup));
		EATEST_VERIFY(get<1>(refTup) == get<1>(constRefTup));
		EATEST_VERIFY(constRefTup == valTup);
		EATEST_VERIFY(constRefTup == refTup);

		// swap
		swap(lesserTuple, greaterTuple);
		EATEST_VERIFY(get<2>(lesserTuple) == 4 && get<2>(greaterTuple) == 3);
		swap(greaterTuple, lesserTuple);
		EATEST_VERIFY(lesserTuple < greaterTuple);
	}

	{
		// Test construction of tuple containing a move only type
		static_assert(is_constructible<MoveOnlyType, MoveOnlyType>::value, "is_constructible type trait giving confusing answers.");
		static_assert(is_constructible<MoveOnlyType, MoveOnlyType&&>::value, "is_constructible type trait giving wrong answers.");
		static_assert(is_constructible<MoveOnlyType&&, MoveOnlyType&&>::value, "is_constructible type trait giving bizarre answers.");
		tuple<MoveOnlyType> aTupleWithMoveOnlyMember(1);
		EATEST_VERIFY(get<0>(aTupleWithMoveOnlyMember).mVal == 1);
		get<0>(aTupleWithMoveOnlyMember) = MoveOnlyType(2);
		EATEST_VERIFY(get<0>(aTupleWithMoveOnlyMember).mVal == 2);

		tuple<const MoveOnlyType&> aTupleWithRefToMoveOnlyMember(aTupleWithMoveOnlyMember);
		EATEST_VERIFY(get<0>(aTupleWithRefToMoveOnlyMember).mVal == 2);

		tuple<const MoveOnlyType&> aTupleWithConstRefToGetMoveOnly(get<0>(aTupleWithMoveOnlyMember));
		EATEST_VERIFY(get<0>(aTupleWithConstRefToGetMoveOnly).mVal == 2);

		tuple<MoveOnlyType&> aTupleWithRefToGetMoveOnly(get<0>(aTupleWithMoveOnlyMember));
		EATEST_VERIFY(get<0>(aTupleWithRefToGetMoveOnly).mVal == 2);
	}

	{
		// Test construction of tuple containing r-value references
		int x = 42;
		TestObject object{1337};
		
		tuple<int&&, TestObject&&> aTupleWithRValueReference(eastl::move(x), eastl::move(object));
		static_assert(is_same<decltype(get<0>(aTupleWithRValueReference)), int&>::value, "wrong return type for get when using r-value reference.");
		static_assert(is_same<decltype(get<1>(aTupleWithRValueReference)), TestObject&>::value, "wrong return type for get when using r-value reference.");
		EATEST_VERIFY(get<0>(aTupleWithRValueReference) == 42);
		EATEST_VERIFY(get<1>(aTupleWithRValueReference).mX == 1337);

		static_assert(!is_constructible<decltype(aTupleWithRValueReference), int&, TestObject&>::value, "it shouldn't be possible to assign r-value references with l-values.");
	}

	{
		// Tuple helpers

		// make_tuple
		auto makeTuple = make_tuple(1, 2.0, true);
		EATEST_VERIFY(get<0>(makeTuple) == 1 && get<1>(makeTuple) == 2.0 && get<2>(makeTuple) == true);

		// TODO: reference_wrapper implementation needs to be finished to enable this code
		{
			int a = 2;
			float b = 3.0f;
			auto makeTuple2 = make_tuple(ref(a), b);
			get<0>(makeTuple2) = 3;
			get<1>(makeTuple2) = 4.0f;
			EATEST_VERIFY(get<0>(makeTuple2) == 3 && get<1>(makeTuple2) == 4.0f && a == 3 && b == 3.0f);
		}

		// forward_as_tuple
		{
			auto forwardTest = [](tuple<MoveOnlyType&&, MoveOnlyType&&> x) -> tuple<MoveOnlyType, MoveOnlyType>
			{
				return tuple<MoveOnlyType, MoveOnlyType>(move(x));
			};

			tuple<MoveOnlyType, MoveOnlyType> aMovableTuple(
			    forwardTest(forward_as_tuple(MoveOnlyType(1), MoveOnlyType(2))));

			EATEST_VERIFY(get<0>(aMovableTuple).mVal == 1 && get<1>(aMovableTuple).mVal == 2);
		}

		{
			// tie
			int a = 0;
			double b = 0.0f;
			static_assert(is_assignable<const Internal::ignore_t&, int>::value, "ignore_t not assignable");
			static_assert(noexcept(ignore = 1), "ignore_t assignment not noexcept");
			static_assert(&(ignore = 1) == &ignore, "ignore_t assignment not constexpr");
			static_assert(Internal::TupleAssignable<tuple<const Internal::ignore_t&>, tuple<int>>::value, "Not assignable");
			tie(a, ignore, b) = make_tuple(1, 3, 5);
			EATEST_VERIFY(a == 1 && b == 5.0f);

			auto aCattedRefTuple = tuple_cat(make_tuple(1), tie(a, ignore, b));
			get<1>(aCattedRefTuple) = 2;
			EATEST_VERIFY(a == 2);
		}

		// tuple_cat
		{
			// zero args
			{				
				auto result = tuple_cat();
				static_assert(eastl::is_same_v<decltype(result), tuple<>>, "type mismatch");
			}

			// one arg - l-value
			{
				tuple<int, bool> t{42, true};
				
				auto result = tuple_cat(t);
				
				static_assert(eastl::is_same_v<decltype(result), tuple<int, bool>>, "type mismatch");
				
				EATEST_VERIFY(get<0>(result) == 42);
				EATEST_VERIFY(get<1>(result));
			}

			// one arg - r-value
			{
				tuple<int, unique_ptr<bool>> t{42, new bool(true)};
				
				auto result = tuple_cat(eastl::move(t));

				static_assert(eastl::is_same_v<decltype(result), tuple<int, unique_ptr<bool>>>, "type mismatch");
				
				EATEST_VERIFY(get<0>(result) == 42);
				EATEST_VERIFY(get<1>(result) != nullptr && *get<1>(result));
				EATEST_VERIFY(get<1>(t) == nullptr);
			}
			
			// two args - l-values
			{
				tuple<int, bool> t1{42, true};
				tuple<float, int> t2{3.14f, 1337};
				
				auto result = tuple_cat(t1, t2);

				static_assert(eastl::is_same_v<decltype(result), tuple<int, bool, float, int>>, "type mismatch");
				
				EATEST_VERIFY(get<0>(result) == 42);
				EATEST_VERIFY(get<1>(result));
				EATEST_VERIFY(get<2>(result) == 3.14f);
				EATEST_VERIFY(get<3>(result) == 1337);
			}

			// two args - r-values
			{
				tuple<int, unique_ptr<bool>> t1{42, new bool(true)};
				tuple<unique_ptr<float>, int> t2{new float(3.14f), 1337};
				
				auto result = tuple_cat(eastl::move(t1), eastl::move(t2));

				static_assert(eastl::is_same_v<decltype(result), tuple<int, unique_ptr<bool>, unique_ptr<float>, int>>, "type mismatch");
				
				EATEST_VERIFY(get<0>(result) == 42);
				EATEST_VERIFY(get<1>(result) != nullptr && *get<1>(result));
				EATEST_VERIFY(get<1>(t1) == nullptr);
				EATEST_VERIFY(get<2>(result) != nullptr && *get<2>(result) == 3.14f);
				EATEST_VERIFY(get<3>(result) == 1337);
				EATEST_VERIFY(get<0>(t2) == nullptr);
			}

			// More than two parameters and empty tuples.
			{
				tuple<int, bool> t1{42, true};
				tuple<unique_ptr<float>, int> t2{new float(3.14f), 1337};
				tuple<> t3{};
				tuple<unique_ptr<short>> t4{new short(10)};
				
				auto result = tuple_cat(t1, eastl::move(t2), t3, eastl::move(t4));

				static_assert(eastl::is_same_v<decltype(result), tuple<int, bool, unique_ptr<float>, int, unique_ptr<short>>>, "type mismatch");
				
				EATEST_VERIFY(get<0>(result) == 42);
				EATEST_VERIFY(get<1>(result));
				EATEST_VERIFY(get<2>(result) != nullptr && *get<2>(result) == 3.14f);
				EATEST_VERIFY(get<3>(result) == 1337);
				EATEST_VERIFY(get<4>(result) != nullptr && *get<4>(result) == 10);
			}
		}

		{
			// Empty tuple
			tuple<> emptyTuple;
			EATEST_VERIFY(tuple_size<decltype(emptyTuple)>::value == 0);
			emptyTuple = make_tuple();
			auto anotherEmptyTuple = make_tuple();
			swap(anotherEmptyTuple, emptyTuple);
		}
	}

	// test piecewise_construct
	{
		{
			struct local
			{
				local() = default;
				local(int a, int b) : mA(a), mB(b) {}

				int mA = 0;
				int mB = 0;
			};

			auto t = make_tuple(42, 43);

			eastl::pair<local, local> p(eastl::piecewise_construct, t, t);

			EATEST_VERIFY(p.first.mA  == 42);
			EATEST_VERIFY(p.second.mA == 42);
			EATEST_VERIFY(p.first.mB  == 43);
			EATEST_VERIFY(p.second.mB == 43);
		}

		{
			struct local
			{
				local() = default;
				local(int a, int b, int c, int d) : mA(a), mB(b), mC(c), mD(d) {}

				int mA = 0;
				int mB = 0;
				int mC = 0;
				int mD = 0;
			};

			auto t = make_tuple(42, 43, 44, 45);

			eastl::pair<local, local> p(eastl::piecewise_construct, t, t);

			EATEST_VERIFY(p.first.mA  == 42);
			EATEST_VERIFY(p.second.mA == 42);

			EATEST_VERIFY(p.first.mB  == 43);
			EATEST_VERIFY(p.second.mB == 43);

			EATEST_VERIFY(p.first.mC  == 44);
			EATEST_VERIFY(p.second.mC == 44);

			EATEST_VERIFY(p.first.mD  == 45);
			EATEST_VERIFY(p.second.mD == 45);
		}

		{
			struct local1
			{
				local1() = default;
				local1(int a) : mA(a) {}
				int mA = 0;
			};

			struct local2
			{
				local2() = default;
				local2(char a) : mA(a) {}
				char mA = 0;
			};

			auto t1 = make_tuple(42);
			auto t2 = make_tuple('a');

			eastl::pair<local1, local2> p(eastl::piecewise_construct, t1, t2);

			EATEST_VERIFY(p.first.mA  == 42);
			EATEST_VERIFY(p.second.mA == 'a');
		}
	}

	// apply
	{
		// test with tuples
		{
			{
				auto result = eastl::apply([](int i) { return i; }, make_tuple(1));
				EATEST_VERIFY(result == 1);
			}

			{
				auto result = eastl::apply([](int i, int j) { return i + j; }, make_tuple(1, 2));
				EATEST_VERIFY(result == 3);
			}


			{
				auto result = eastl::apply([](int i, int j, int k, int m) { return i + j + k + m; }, make_tuple(1, 2, 3, 4));
				EATEST_VERIFY(result == 10);
			}
		}

		// test with pair
		{
			auto result = eastl::apply([](int i, int j) { return i + j; }, make_pair(1, 2));
			EATEST_VERIFY(result == 3);
		}

		// test with array
		{
			// TODO(rparolin):   
			// eastl::array requires eastl::get support before we can support unpacking eastl::arrays with eastl::apply.
			//
			// {
			//     auto result = eastl::apply([](int i) { return i; }, eastl::array<int, 1>{1});
			//     EATEST_VERIFY(result == 1);
			// }
			// {
			//     auto result = eastl::apply([](int i, int j) { return i + j; }, eastl::array<int, 2>{1,2});
			//     EATEST_VERIFY(result == 3);
			// }
			// {
			//     auto result = eastl::apply([](int i, int j, int k, int m) { return i + j + k + m; }, eastl::array<int, 4>{1, 2, 3, 4});
			//     EATEST_VERIFY(result == 10);
			// }
		}
	}

	// make_from_tuple
	{
		{
			TestObject x = eastl::make_from_tuple<TestObject>(eastl::make_tuple(1, true));
			EATEST_VERIFY(x.mX == 1);
			EATEST_VERIFY(x.mbThrowOnCopy == true);
		}

		{
			TestObject x = eastl::make_from_tuple<TestObject>(eastl::make_tuple(1, 2, 3));
			EATEST_VERIFY(x.mX == 6);
		}

		{
			int i = 7;
			const int& x = eastl::make_from_tuple<const int&>(eastl::tuple<int&>(i));
			EATEST_VERIFY(&x == &i);
			// make_from_tuple<const int&>(tuple<long>(7)) doesn't compile: it would bind x to a temporary.
		}
	}

	// A reference element is never bound to a temporary: C++23's rule, applied in every language mode, wherever the
	// compiler can tell (a compiler without the intrinsic accepts these as before).
	{
		constexpr bool detected = Internal::reference_constructs_from_temporary_v<const int&, long&&>;
		static_assert(!detected || !is_constructible_v<tuple<const int&>, long>, "tuple(U&&...) is deleted");
		static_assert(!detected || !is_constructible_v<tuple<int, const int&>, int, long&>, "tuple(U&&...) is deleted");
		static_assert(!detected || !is_constructible_v<tuple<const int&>, tuple<long>>, "tuple(tuple<U...>&&)");
		static_assert(!detected || !is_constructible_v<tuple<const int&>, const tuple<long>&>, "tuple(const tuple<U...>&)");
		static_assert(!detected || !is_constructible_v<tuple<const int&, int>, pair<long, int>>, "tuple(pair<U1, U2>&&)");
		static_assert(is_constructible_v<tuple<const int&>, const int&>, "binds to the argument");
		static_assert(is_constructible_v<tuple<const int&>, tuple<int>&>, "binds to the source's element");

		const int one = 1;
		tuple<const int&, int> t(one, 2);
		tuple<int> source(3);
		tuple<const int&> u(source);
		EATEST_VERIFY((&get<0>(t) == &one) && (&get<0>(u) == &get<0>(source)));

		// A FromAnything could be built from the tuple through a temporary, but copying the tuple copies the reference.
		FromAnything value;
		tuple<const FromAnything&> referenceTuple(value);
		tuple<const FromAnything&> referenceTupleCopy(referenceTuple);
		EATEST_VERIFY(&get<0>(referenceTupleCopy) == &value);
	}

	// Compilation test to make sure that the conditionally-explicit cast works
	{
		eastl::tuple<int, float, TestObject> arrayTup[] = {
			{1, 1.0f, TestObject(1)},
			{2, 2.0f, TestObject(2)},
			{3, 3.0f, TestObject(3)},
			{4, 4.0f, TestObject(4)}
		};
		(void)arrayTup;

#if false
		// the following code should not compile with conditionally-explicit behaviour (but does with fully implicit behaviour)
		eastl::tuple<eastl::vector<float>, float> arrayOfArrayTup[] = {
			{1.0f, 1.0f},
			{2.0f, 2.0f}
		};

		eastl::tuple<eastl::vector<int>, float> arrayOfArrayTup2[] = {
			{1, 1.0f},
			{2, 2.0f}
		};
#endif
	}

	// The default constructor takes part only when every element is default constructible, and it's explicit when an
	// element's default constructor is, like std::tuple.
	{
		struct ExplicitDefault { explicit ExplicitDefault() = default; };
		struct NoDefault { int mX; NoDefault(int x) : mX(x) {} };

		static_assert(!is_default_constructible_v<tuple<int&>>, "tuple<int&> is not default constructible");
		static_assert(!is_default_constructible_v<tuple<NoDefault>>, "tuple<NoDefault> is not default constructible");
		static_assert(!is_default_constructible_v<tuple<int, NoDefault>>, "tuple<int, NoDefault> is not default constructible");
		static_assert(is_default_constructible_v<tuple<ExplicitDefault>>, "tuple<ExplicitDefault> is default constructible");

		static_assert(Internal::is_implicitly_default_constructible<tuple<int, float>>::value, "tuple<int, float>() is implicit");
		static_assert(!Internal::is_implicitly_default_constructible<tuple<ExplicitDefault>>::value, "tuple<ExplicitDefault>() is explicit");
		static_assert(!Internal::is_implicitly_default_constructible<tuple<int, ExplicitDefault>>::value, "tuple<int, ExplicitDefault>() is explicit");

		tuple<int, ExplicitDefault, float> t{};
		EATEST_VERIFY((get<0>(t) == 0) && (get<2>(t) == 0.f));
	}

	// Compilation test to make sure that we can handle reference to forward-declared types
	{
		struct ForwardDeclared;

		auto fill_tuple = [](ForwardDeclared& f) {
			eastl::tuple<ForwardDeclared&, const ForwardDeclared&> t{f, f};
			return t;
		};

		struct ForwardDeclared
		{
			int x;
		};

		ForwardDeclared f{666};
		auto t = fill_tuple(f);

		EATEST_VERIFY(get<0>(t).x == 666);
		EATEST_VERIFY(get<1>(t).x == 666);
	}

	#ifndef EA_COMPILER_NO_STRUCTURED_BINDING
	// tuple structured bindings test 
	{
		eastl::tuple<int, int, int> t = {1,2,3};
		auto [x,y,z] = t;
		EATEST_VERIFY(x == 1);
		EATEST_VERIFY(y == 2);
		EATEST_VERIFY(z == 3);
	}

	{ // const unpacking test
		eastl::tuple<int, int, int> t = {1,2,3};
		const auto [x,y,z] = t;
		EATEST_VERIFY(x == 1);
		EATEST_VERIFY(y == 2);
		EATEST_VERIFY(z == 3);
	}
	#endif

	// user reported regression that exercises type_traits trying to pull out the element_type from "fancy pointers"
	{
		auto up = eastl::make_unique<int[]>(100);
		auto t = eastl::make_tuple(eastl::move(up));

		using ResultTuple_t = decltype(t);
		static_assert(eastl::is_same_v<ResultTuple_t, eastl::tuple<eastl::unique_ptr<int[]>>>); 
		static_assert(eastl::is_same_v<eastl::tuple_element_t<0, ResultTuple_t>, eastl::unique_ptr<int[]>>);
	}

	// user reported issue that a tuple was not default constructible if a tuple element type had no members (ie. is_empty_v<T>).
	{
		tuple<> emptyTuple;
		EA_UNUSED(emptyTuple);

		tuple<NoDataMembers> tupleWithEmptyMember;
		EA_UNUSED(tupleWithEmptyMember);
		tuple<NoDataMembers, int> tupleWithEmptyMember2;
		EA_UNUSED(tupleWithEmptyMember2);
		tuple<int, NoDataMembers> tupleWithEmptyMember3;
		EA_UNUSED(tupleWithEmptyMember3);

		tuple<NoDataMembers> tupleWithEmptyMember4(NoDataMembers{});
		EA_UNUSED(tupleWithEmptyMember4);

		struct EmptyNoDefaultCtor
		{
			EmptyNoDefaultCtor() = delete;
			EmptyNoDefaultCtor(int) {}
		};

		tuple<EmptyNoDefaultCtor> tupleWithEmptyMember5(EmptyNoDefaultCtor{3});
		EA_UNUSED(tupleWithEmptyMember5);
	}

	// tests for an empty nested tuple element.
	// Empty tuples as elements of a tuple caused compilation errors in get<I>(some-tuple) in an earlier version.
	{
		EATEST_VERIFY(get<0>(tuple<tuple<>>{}) == tuple<>{});
		EATEST_VERIFY(get<0>(tuple<int, tuple<>>{}) == 0);
		EATEST_VERIFY(get<1>(tuple<int, tuple<>>{}) == tuple<>{});
		EATEST_VERIFY(get<0>(tuple<tuple<>, int>{}) == tuple<>{});
		EATEST_VERIFY(get<1>(tuple<tuple<>, int>{}) == 0);

		EATEST_VERIFY(get<tuple<>>(tuple<tuple<>>{}) == tuple<>{});
		EATEST_VERIFY(get<int>(tuple<int, tuple<>>{}) == 0);
		EATEST_VERIFY(get<tuple<>>(tuple<int, tuple<>>{}) == tuple<>{});
		EATEST_VERIFY(get<tuple<>>(tuple<tuple<>, int>{}) == tuple<>{});
		EATEST_VERIFY(get<int>(tuple<tuple<>, int>{}) == 0);

		// test special member functions.
		tuple<tuple<>, int> a;
		tuple<tuple<>, int> b(a);
		EATEST_VERIFY(b == a);
		b = a;
		EATEST_VERIFY(b == a);
		b = eastl::move(a);
		EATEST_VERIFY(b == a);
	}

	{

		// check that we can assign from/to tuples of references.
		int a{1};
		int b{2};
		tuple<int&> t1{a};
		tuple<int&> t2{b};
		EATEST_VERIFY(get<0>(t1) == 1);
		EATEST_VERIFY(get<0>(t2) == 2);

		t1 = eastl::move(t2);
		EATEST_VERIFY(get<0>(t1) == 2);
		EATEST_VERIFY(a == 2);

		get<0>(t2) = 3;
		t1 = t2;
		EATEST_VERIFY(get<0>(t1) == 3);
		EATEST_VERIFY(a == 3);
		EATEST_VERIFY(b == 3);

		a = 1;
		EATEST_VERIFY(get<0>(t1) == 1);

		const tuple<int&> t3{b};
		t1 = t3;
		EATEST_VERIFY(get<0>(t1) == 3);
	}

	// Move-assigning a tuple<T&> assigns each referenced object from an lvalue, like std::tuple: the source objects are
	// copied, not moved from, and an element type whose move assignment is deleted is fine.
	{
		TestObject::Reset();
		{
			TestObject a(1), b(2);
			const int64_t moves = TestObject::sTOMoveAssignCount, copies = TestObject::sTOCopyAssignCount;
			eastl::tie(a) = eastl::tie(b);
			tuple<TestObject&> ta(a), tb(b);
			ta = eastl::move(tb);
			EATEST_VERIFY((a.mX == 2) && (b.mX == 2));
			EATEST_VERIFY((TestObject::sTOMoveAssignCount == moves) && (TestObject::sTOCopyAssignCount == copies + 2));
		}
		EATEST_VERIFY(TestObject::IsClear());
		TestObject::Reset();

		struct NoMoveAssign
		{
			int mX = 0;
			NoMoveAssign() = default;
			NoMoveAssign& operator=(const NoMoveAssign&) = default;
			NoMoveAssign& operator=(NoMoveAssign&&) = delete;
		};
		static_assert(is_move_assignable_v<tuple<NoMoveAssign&>>, "tuple<NoMoveAssign&> is move assignable");
		NoMoveAssign x, y;
		y.mX = 3;
		tuple<NoMoveAssign&> tx(x), ty(y);
		tx = eastl::move(ty);
		EATEST_VERIFY(x.mX == 3);
	}

	// C++23's const swap swaps the objects that a tuple of references refers to, including through temporaries like
	// the tuple_vector iterator's reference. A tuple of values can't be swapped as const.
	{
		static_assert(is_swappable_v<const tuple<int&, long&>>, "const tuple<int&, long&> is swappable");
		static_assert(is_swappable_with_v<tuple<int&>, tuple<int&>>, "rvalue tuple<int&> is swappable");
		static_assert(!is_swappable_v<const tuple<int, long>>, "const tuple<int, long> isn't swappable");

		int a = 1, b = 2;
		long c = 3, d = 4;
		const tuple<int&, long&> t1(a, c), t2(b, d);
		swap(t1, t2);
		EATEST_VERIFY((a == 2) && (b == 1) && (c == 4) && (d == 3));
		t1.swap(t2);
		swap(tie(a, c), tie(b, d));
		EATEST_VERIFY((a == 2) && (b == 1) && (c == 4) && (d == 3));

	#if defined(EA_COMPILER_CPP20_ENABLED)
		constexpr auto constSwapped = []
		{
			int x = 1, y = 2;
			const tuple<int&> r1(x), r2(y);
			r1.swap(r2);
			swap(r1, r2);
			r1.swap(r2);
			const tuple<> e1{}, e2{};
			e1.swap(e2);
			return (x == 2) && (y == 1);
		};
		static_assert(constSwapped(), "constexpr const swap");
	#endif
	}

	// copy and move are deleted when an element can't be copied or moved, like std::tuple.
	{
		static_assert(!is_copy_assignable_v<tuple<const int>>, "tuple<const int> is not copy assignable");
		static_assert(!is_move_assignable_v<tuple<const int>>, "tuple<const int> is not move assignable");
		static_assert(!is_copy_assignable_v<tuple<const int&>>, "tuple<const int&> is not copy assignable");
		static_assert(is_copy_assignable_v<tuple<int&>>, "tuple<int&> is copy assignable");

		static_assert(!is_move_constructible_v<tuple<NoCopyMove>>, "tuple<NoCopyMove> is not move constructible");
		static_assert(!is_move_assignable_v<tuple<NoCopyMove>>, "tuple<NoCopyMove> is not move assignable");
		static_assert(!is_swappable_v<tuple<NoCopyMove>>, "tuple<NoCopyMove> is not swappable");
		static_assert(!is_move_constructible_v<tuple<NoCopyMoveNonEmpty>>, "tuple<NoCopyMoveNonEmpty> is not move constructible");
		static_assert(!is_move_assignable_v<tuple<NoCopyMoveNonEmpty>>, "tuple<NoCopyMoveNonEmpty> is not move assignable");
		static_assert(!is_move_constructible_v<tuple<DeletedMoveFromAnything>>, "tuple<DeletedMoveFromAnything> is not move constructible");
		static_assert(!is_move_constructible_v<tuple<DeletedMoveFromAnythingNonEmpty>>, "tuple<DeletedMoveFromAnythingNonEmpty> is not move constructible");
		static_assert(!is_move_constructible_v<tuple<int, DeletedMoveFromAnythingNonEmpty>>, "tuple<int, DeletedMoveFromAnythingNonEmpty> is not move constructible");
		static_assert(is_move_constructible_v<tuple<int, MoveOnlyType>> && !is_copy_constructible_v<tuple<int, MoveOnlyType>>, "tuple<int, MoveOnlyType> is only moved");
		static_assert(is_copy_constructible_v<tuple<int, TestObject>> && is_move_constructible_v<tuple<int, TestObject>>, "tuple<int, TestObject> is copied and moved");

		static_assert(is_trivially_copy_constructible_v<tuple<int>>, "EASTL extension, tuple<int> is trivially copy constructible.");
		static_assert(is_trivially_destructible_v<tuple<int>>, "tuple<int> is trivially destructible");
	}

	// The converting assignment assigns to each element as an lvalue, like std::tuple, without constructing any.
	{
		// The declared default constructors keep these from being aggregates, which C++20 could build from a number.
		struct LvalueAssign { int mX = 0; LvalueAssign() = default; LvalueAssign& operator=(short x) & { mX = x; return *this; } };
		struct RvalueAssign { RvalueAssign& operator=(short) && { return *this; } };
		struct AssignFromInt { int mX = 0; AssignFromInt() = default; AssignFromInt& operator=(int x) { mX = x; return *this; } };

		static_assert(is_assignable_v<tuple<LvalueAssign>&, const tuple<short>&>, "LvalueAssign is assigned as an lvalue");
		static_assert(!is_assignable_v<tuple<RvalueAssign>&, const tuple<short>&>, "RvalueAssign can't be assigned as an lvalue");
		static_assert(is_assignable_v<tuple<long, AssignFromInt>&, tuple<int, int>>, "AssignFromInt is assigned, not constructed");
		static_assert(is_assignable_v<tuple<long, AssignFromInt>&, pair<int, int>>, "AssignFromInt is assigned from a pair");

		tuple<LvalueAssign> a;
		a = tuple<short>(short(1));
		EATEST_VERIFY(get<0>(a).mX == 1);

		tuple<long, AssignFromInt> b;
		b = make_tuple(2, 3);
		EATEST_VERIFY((get<0>(b) == 2) && (get<1>(b).mX == 3));
		b = make_pair(4, 5);
		EATEST_VERIFY((get<0>(b) == 4) && (get<1>(b).mX == 5));
	}

	// A volatile tuple or pair can't be read with get, so it can't be converted or assigned from, like for std::tuple.
	{
		static_assert(!is_constructible_v<tuple<int, short>, volatile tuple<int, short>&>, "copy from a volatile tuple");
		static_assert(!is_constructible_v<tuple<long, short>, volatile tuple<int, short>&>, "conversion from a volatile tuple");
		static_assert(!is_constructible_v<tuple<long, short>, const volatile pair<int, short>&>, "conversion from a volatile pair");
		static_assert(!is_assignable_v<tuple<int, short>&, volatile tuple<int, short>&>, "copy assignment from a volatile tuple");
		static_assert(!is_assignable_v<tuple<long, short>&, volatile tuple<int, short>&>, "assignment from a volatile tuple");
		static_assert(!is_assignable_v<tuple<long, short>&, volatile pair<int, short>&>, "assignment from a volatile pair");

		static_assert(is_constructible_v<tuple<long, short>, const tuple<int, short>&>, "conversion from a const tuple");
		static_assert(is_constructible_v<tuple<long>, const tuple<volatile int>&>, "conversion from a tuple with a volatile element");
	}

	{
		tuple<NoCopyMove> t;
		[[maybe_unused]] auto& x = get<0>(t);
	}

	{
		tuple<NoCopyMoveNonEmpty> t;
		[[maybe_unused]] auto& x = get<0>(t);
	}

	{
		NoCopyMove x;
		tuple<NoCopyMove&> t{x};
		[[maybe_unused]] auto& y = get<0>(t);
	}

	{
		// Check that this still compiles:
		NoCopyMoveNonEmpty x;
		tuple<NoCopyMoveNonEmpty&> t{x};
		[[maybe_unused]] auto& y = get<0>(t);
	}

	// Empty class types, final or not, are stored as members, so distinct elements of the same type have distinct
	// addresses. With the Empty Base Class Optimization, the Microsoft ABI gave them the same address.
	{
		struct EmptyTag {};
		struct FinalTag final { int Value() const { return 7; } };
		struct HasTag { EmptyTag mTag; };

		tuple<FinalTag> t1;
		EATEST_VERIFY(get<0>(t1).Value() == 7);

		tuple<FinalTag, int, FinalTag> t2(FinalTag(), 5, FinalTag());
		EATEST_VERIFY(get<1>(t2) == 5);
		EATEST_VERIFY(get<2>(t2).Value() == 7);

		// Compare through volatile so the compiler can't assume distinct same-type objects have distinct addresses.
		auto distinct = [](const void* a, const void* b)
		{
			volatile uintptr_t addressA = reinterpret_cast<uintptr_t>(a);
			volatile uintptr_t addressB = reinterpret_cast<uintptr_t>(b);
			return addressA != addressB;
		};

		EATEST_VERIFY(distinct(&get<0>(t2), &get<2>(t2)));

		tuple<EmptyTag, tuple<EmptyTag>> nested;
		EATEST_VERIFY(distinct(&get<0>(nested), &get<0>(get<1>(nested))));

		tuple<EmptyTag, HasTag> member;
		EATEST_VERIFY(distinct(&get<0>(member), &get<1>(member).mTag));

		tuple<EmptyTag, EmptyTag> adjacent[2];
		EATEST_VERIFY(distinct(&get<0>(adjacent[0]), &get<1>(adjacent[0])));
		EATEST_VERIFY(distinct(&get<1>(adjacent[0]), &get<0>(adjacent[1])));

		static_assert(is_trivially_copy_constructible_v<tuple<EmptyTag, FinalTag>>, "trivial copy construction");
		constexpr tuple<EmptyTag> constantEmpty{};
		EA_UNUSED(constantEmpty);
	}

	nErrorCount += TestTupleIncludedFirstComparisons();

	return nErrorCount;
}

#else

int TestTuple() { return 0; }

#endif  // EASTL_TUPLE_ENABLED

EA_RESTORE_VC_WARNING()
