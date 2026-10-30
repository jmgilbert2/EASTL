/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "EASTLTest.h"
#include <EASTL/utility.h>
#include <EASTL/algorithm.h>
#include <EASTL/hash_map.h>
#include <EASTL/map.h>
#include <EASTL/optional.h>
#include <EASTL/sort.h>
#include <EASTL/tuple.h>
#include <EASTL/variant.h>
#include <EASTL/vector.h>
#include <EASTL/vector_map.h>
#include <EAStdC/EAString.h>

EA_DISABLE_ALL_VC_WARNINGS()
#ifndef EA_COMPILER_NO_STANDARD_CPP_LIBRARY
	#include <algorithm>
	#include <chrono>
	#include <functional>
	#include <type_traits>
	#include <utility>
	#include <vector>
#endif
EA_RESTORE_ALL_VC_WARNINGS()

struct BasicObject
{
	int mX;
	BasicObject(int x) : mX(x) {}
};

inline bool operator==(const BasicObject& t1, const BasicObject& t2) { return t1.mX == t2.mX; }

inline bool operator<(const BasicObject& t1, const BasicObject& t2) { return t1.mX < t2.mX; }

static_assert(eastl::is_trivially_copyable_v<eastl::pair<int, int>>, "EASTL extension, pair is trivially copyable.");
static_assert(eastl::is_trivially_copyable_v<eastl::pair<float, int>>, "EASTL extension, pair is trivially copyable.");
static_assert(eastl::is_trivially_copyable_v<eastl::pair<int, float>>, "EASTL extension, pair is trivially copyable.");
static_assert(eastl::is_trivially_copy_assignable_v<eastl::pair<int, int>>, "EASTL extension, pair's copy assignment is trivial.");

///////////////////////////////////////////////////////////////////////////////
// TestUtilityPair
//
static int TestUtilityPair()
{
	using namespace eastl;

	int nErrorCount = 0;

	{
		int _0 = 0, _2 = 2, _3 = 3;
		float _1f = 1.f;

		// pair();
		pair<int, float> ifPair1;
		EATEST_VERIFY((ifPair1.first == 0) && (ifPair1.second == 0.f));

		// pair(const T1& x, const T2& y);
		pair<int, float> ifPair2(_0, _1f);
		EATEST_VERIFY((ifPair2.first == 0) && (ifPair2.second == 1.f));

		// template <typename U, typename V>
		// pair(U&& u, V&& v);
		pair<int, float> ifPair3(int(0), float(1.f));
		EATEST_VERIFY((ifPair3.first == 0) && (ifPair3.second == 1.f));

		// template <typename U>
		// pair(U&& x, const T2& y);
		const float fConst1 = 1.f;
		pair<int, float> ifPair4(int(0), fConst1);
		EATEST_VERIFY((ifPair4.first == 0) && (ifPair4.second == 1.f));

		// template <typename V>
		// pair(const T1& x, V&& y);
		const int intConst0 = 0;
		pair<int, float> ifPair5(intConst0, float(1.f));
		EATEST_VERIFY((ifPair5.first == 0) && (ifPair5.second == 1.f));

		pair<const int, const int> constIntPair(_2, _3);
		EATEST_VERIFY((constIntPair.first == 2) && (constIntPair.second == 3));

		// pair(const pair&) = default;
		pair<int, float> ifPair2Copy(ifPair2);
		EATEST_VERIFY((ifPair2Copy.first == 0) && (ifPair2Copy.second == 1.f));

		pair<const int, const int> constIntPairCopy(constIntPair);
		EATEST_VERIFY((constIntPairCopy.first == 2) && (constIntPairCopy.second == 3));

		// template<typename U, typename V>
		// pair(const pair<U, V>& p);
		pair<long, double> idPair2(ifPair2);
		EATEST_VERIFY((idPair2.first == 0) && (idPair2.second == 1.0));

		// invoke the explicit TestObject constructor:
		// explicit TestObject(int x = 0, bool bThrowOnCopy = false)
		pair<TestObject, int> tiPair1(0, 4);

		// pair(pair&& p);

		// template<typename U, typename V>
		// pair(pair<U, V>&& p);

		// pair& operator=(const pair& p);

		// template<typename U, typename V>
		// pair& operator=(const pair<U, V>& p);

		// pair& operator=(pair&& p);

		// template<typename U, typename V>
		// pair& operator=(pair<U, V>&& p);

		// void swap(pair& p);

		// use_self, use_first, use_second
		use_self<pair<int, float> > usIFPair;
		use_first<pair<int, float> > u1IFPair;
		use_second<pair<int, float> > u2IFPair;

		ifPair2 = usIFPair(ifPair2);
		EATEST_VERIFY((ifPair2.first == 0) && (ifPair2.second == 1));

		int first = u1IFPair(ifPair2);
		EATEST_VERIFY(first == 0);

		float second = u2IFPair(ifPair2);
		EATEST_VERIFY(second == 1);

		// make_pair
		pair<int, float> p1 = make_pair(int(0), float(1));
		EATEST_VERIFY((p1.first == 0) && (p1.second == 1.f));

		EASTL_INTERNAL_DISABLE_DEPRECATED() // make_pair_ref: was declared deprecated
		pair<int, float> p2 = make_pair_ref(int(0), float(1));
		EATEST_VERIFY((p2.first == 0) && (p2.second == 1.f));
		EASTL_INTERNAL_RESTORE_DEPRECATED() // make_pair_ref: was declared deprecated

		pair<const char*, int> p3 = eastl::make_pair("a", 1);
		EATEST_VERIFY((EA::StdC::Strcmp(p3.first, "a") == 0) && (p3.second == 1));

		pair<const char*, int> p4 = eastl::make_pair("a", 1);
		EATEST_VERIFY((EA::StdC::Strcmp(p4.first, "a") == 0) && (p4.second == 1));

		pair<int, const char*> p5 = eastl::make_pair(1, "b");
		EATEST_VERIFY((p5.first == 1) && (EA::StdC::Strcmp(p5.second, "b") == 0));

#if defined(EA_COMPILER_HAS_THREE_WAY_COMPARISON)
		pair<int, int> p6 = eastl::make_pair(1, 2);
		pair<int, int> p7 = eastl::make_pair(2, 1);
		pair<int, int> p8 = eastl::make_pair(7, 8);
		pair<int, int> p9 = eastl::make_pair(10, 1);

		EATEST_VERIFY( (p6 <=> p7) != 0);
		EATEST_VERIFY( (p6 <=> p6) == 0);
		EATEST_VERIFY( (p7 <=> p8) < 0);
		EATEST_VERIFY( (p7 <=> p8) <= 0);
		EATEST_VERIFY( (p9 <=> p8) > 0);
		EATEST_VERIFY( (p9 <=> p8) >= 0);
#endif

#if !defined(EA_COMPILER_NO_AUTO)
		auto p60 = eastl::make_pair("a", "b");  // Different strings of same length of 1.
		EATEST_VERIFY((EA::StdC::Strcmp(p60.first, "a") == 0) && (EA::StdC::Strcmp(p60.second, "b") == 0));

		auto p61 = eastl::make_pair("ab", "cd");  // Different strings of same length > 1.
		EATEST_VERIFY((EA::StdC::Strcmp(p61.first, "ab") == 0) && (EA::StdC::Strcmp(p61.second, "cd") == 0));

		auto p62 = eastl::make_pair("abc", "bcdef");  // Different strings of different length.
		EATEST_VERIFY((EA::StdC::Strcmp(p62.first, "abc") == 0) && (EA::StdC::Strcmp(p62.second, "bcdef") == 0));

		char strA[] = "a";
		auto p70 = eastl::make_pair(strA, strA);
		EATEST_VERIFY((EA::StdC::Strcmp(p70.first, "a") == 0) && (EA::StdC::Strcmp(p70.second, "a") == 0));

		char strBC[] = "bc";
		auto p71 = eastl::make_pair(strA, strBC);
		EATEST_VERIFY((EA::StdC::Strcmp(p71.first, "a") == 0) && (EA::StdC::Strcmp(p71.second, "bc") == 0));

		const char cstrA[] = "a";
		auto p80 = eastl::make_pair(cstrA, cstrA);
		EATEST_VERIFY((EA::StdC::Strcmp(p80.first, "a") == 0) && (EA::StdC::Strcmp(p80.second, "a") == 0));

		const char cstrBC[] = "bc";
		auto p81 = eastl::make_pair(cstrA, cstrBC);
		EATEST_VERIFY((EA::StdC::Strcmp(p81.first, "a") == 0) && (EA::StdC::Strcmp(p81.second, "bc") == 0));
#endif
	}

#if EASTL_TUPLE_ENABLED
	// get<I>(pair&)
	// get<T>(pair&)
	// get<T, U>(pair&)
	{
		pair<int, float> p{2, 3.3f};
		EATEST_VERIFY(get<0>(p) == 2);
		EATEST_VERIFY(get<1>(p) == 3.3f);

		EATEST_VERIFY(get<int>(p) == 2);
		EATEST_VERIFY(get<float>(p) == 3.3f);
		EATEST_VERIFY((get<int, float>(p) == 2));
		EATEST_VERIFY((get<float, int>(p) == 3.3f));

		get<int>(p) = 3;
		EATEST_VERIFY(p.first == 3);
		get<float>(p) = 4.0f;
		EATEST_VERIFY(p.second == 4.0f);

		// no matching overload, index is not in range.
		// get<2>(p);
	}

	// get<I>(const pair&)
	// get<T>(const pair&)
	// get<T, U>(const pair&)
	{
		const pair<int, float> p{ 2, 3.3f };
		EATEST_VERIFY(get<0>(p) == 2);
		EATEST_VERIFY(get<1>(p) == 3.3f);

		EATEST_VERIFY(get<int>(p) == 2);
		EATEST_VERIFY(get<float>(p) == 3.3f);
		EATEST_VERIFY((get<int, float>(p) == 2));
		EATEST_VERIFY((get<float, int>(p) == 3.3f));

		// cannot assign to a const&.
		// get<int>(p) = 3;
		// get<float>(p) = 4.0f;
	}

	// get<I>(pair&&)
	// get<T>(pair&&)
	// get<T, U>(pair&&)
	{
		EATEST_VERIFY(get<0>(pair<int, MoveOnlyType>{ 2, MoveOnlyType(3) }) == 2);
		MoveOnlyType mo = get<1>(pair<int, MoveOnlyType>{ 2, MoveOnlyType(3) });
		EATEST_VERIFY(mo == MoveOnlyType(3));

		EATEST_VERIFY(get<int>(pair<int, MoveOnlyType>{ 2, MoveOnlyType(3) }) == 2);
		EATEST_VERIFY(get<MoveOnlyType>(pair<int, MoveOnlyType>{ 2, MoveOnlyType(3) }) == MoveOnlyType(3));
		EATEST_VERIFY((get<int, MoveOnlyType>(pair<int, MoveOnlyType>{ 2, MoveOnlyType(3) }) == 2));
		EATEST_VERIFY((get<MoveOnlyType, int>(pair<int, MoveOnlyType>{ 2, MoveOnlyType(3) }) == MoveOnlyType(3)));
	}

	// get<I>(const pair&&)
	// get<T>(const pair&&)
	// get<T, U>(const pair&&)
	{
		using pair_type = const pair<int, MoveOnlyType>;
		EATEST_VERIFY(get<0>(pair_type{ 2, MoveOnlyType(3) }) == 2);
		EATEST_VERIFY(get<1>(pair_type{ 2, MoveOnlyType(3) }) == MoveOnlyType(3));

		EATEST_VERIFY(get<int>(pair_type{ 2, MoveOnlyType(3) }) == 2);
		EATEST_VERIFY(get<MoveOnlyType>(pair_type{ 2, MoveOnlyType(3) }) == MoveOnlyType(3));
		EATEST_VERIFY((get<int, MoveOnlyType>(pair_type{ 2, MoveOnlyType(3) }) == 2));
		EATEST_VERIFY((get<MoveOnlyType, int>(pair_type{ 2, MoveOnlyType(3) }) == MoveOnlyType(3)));
	}

	// get(pair<T, T>)
	{
		pair<int, int> p{ 1, 2 };
		EATEST_VERIFY(get<0>(p) == 1);
		EATEST_VERIFY(get<1>(p) == 2);

		get<0>(p) = 3;
		EATEST_VERIFY(p.first == 3);
		get<1>(p) = 4;
		EATEST_VERIFY(p.second == 4);

		// no matching overload, ambiguous which element.
		// get<int>(p)
	}

	// get<I>(pair) and get<T>(pair) are constexpr
	{
		constexpr pair<int, float> cp{ 1, 2.0f };
		constexpr int ci = get<0>(cp);
		constexpr float cf = get<float>(cp);
		static_assert(ci == 1, "unexpected return from get<I>(pair)");
		static_assert(cf == 2.0f, "unexpected return from get<T>(pair)");
	}


#endif

	{
// One-off tests and regressions

#if EASTL_PAIR_CONFORMANCE  // See http://www.open-std.org/jtc1/sc22/wg21/docs/lwg-defects.html#811
		pair<char*, char*> zeroLiteralPair(0, 0);
		EATEST_VERIFY((zeroLiteralPair.first == NULL) && (zeroLiteralPair.second == NULL));
#endif

		// template<typename U>
		// pair(U&& x, const T2& y)
		typedef eastl::pair<uint16_t, const char8_t*> LCIDMapping;
		LCIDMapping lcidMappingArray[1] = {LCIDMapping(0x0036, EA_CHAR8("af"))};  // Note that 0x0036 is of type int.
		EATEST_VERIFY((lcidMappingArray[0].first == 0x0036));

		// template<typename V>
		// pair(const T1& x, V&& y)
		typedef eastl::pair<const char8_t*, uint16_t> LCIDMapping2;
		LCIDMapping2 lcidMapping2Array[1] = {LCIDMapping2(EA_CHAR8("af"), 0x0036)};
		EATEST_VERIFY((lcidMapping2Array[0].second == 0x0036));

// The following code was giving an EDG compiler:
//       error 145: a value of type "int" cannot be used to initialize
//       an entity of type "void *" second(eastl::forward<V>(v)) {}
// template <typename U, typename V>
// pair(U&& u, V&& v);
#if EASTL_PAIR_CONFORMANCE
		typedef eastl::pair<float*, void*> TestPair1;
		float fOne = 1.f;
		TestPair1 testPair1(&fOne, NULL);
		EATEST_VERIFY(*testPair1.first == 1.f);
#endif
	}

	// The converting constructors are explicit exactly when an element conversion isn't implicit, like std::pair.
	{
		static_assert(eastl::is_convertible_v<pair<int, float>, pair<long, double>>, "pair(pair<U, V>&&) is implicit");
		static_assert(eastl::is_convertible_v<const pair<int, float>&, pair<long, double>>, "pair(const pair<U, V>&) is implicit");
		static_assert(!eastl::is_convertible_v<pair<int, int>, pair<int, TestObject>>, "pair(pair<U, V>&&) is explicit");
		static_assert(!eastl::is_convertible_v<const pair<int, int>&, pair<int, TestObject>>, "pair(const pair<U, V>&) is explicit");

		pair<int, TestObject> p1(pair<int, int>(1, 2));
		EATEST_VERIFY((p1.first == 1) && (p1.second.mX == 2));

		const pair<int, int> p2(3, 4);
		pair<int, TestObject> p3(p2);
		EATEST_VERIFY((p3.first == 3) && (p3.second.mX == 4));

		pair<long, double> p4 = {1, 2.f};
		EATEST_VERIFY((p4.first == 1) && (p4.second == 2.0));
	}

	// A reference element is never bound to a temporary: C++23's rule, applied in every language mode, wherever the
	// compiler can tell (a compiler without the intrinsic accepts these as before).
	{
		constexpr bool detected = eastl::Internal::reference_constructs_from_temporary_v<const int&, long&&>;
		static_assert(!detected || !eastl::is_constructible_v<pair<const int&, int>, long, int>, "pair(U&&, V&&) is deleted");
		static_assert(!detected || !eastl::is_constructible_v<pair<int, const int&>, int, long&>, "pair(U&&, V&&) is deleted");
		static_assert(!detected || !eastl::is_constructible_v<pair<const int&, int>, pair<long, int>>, "pair(pair<U, V>&&)");
		static_assert(!detected || !eastl::is_constructible_v<pair<const int&, int>, const pair<long, int>&>, "pair(const pair<U, V>&)");
		static_assert(eastl::is_constructible_v<pair<const int&, int>, const int&, int>, "binds to the argument");
		static_assert(eastl::is_constructible_v<pair<const int&, int>, pair<int, int>&>, "binds to the source's element");

		const int one = 1;
		pair<const int&, int> p(one, 2);
		pair<int, int> source(3, 4);
		pair<const int&, int> q(source);
		EATEST_VERIFY((&p.first == &one) && (&q.first == &source.first));
	}

#if defined(__cpp_concepts) && __cpp_concepts >= 202002L
	// pair<T&, U> is copy assignable and assigns through the reference, like std::pair.
	{
		static_assert(eastl::is_copy_assignable_v<pair<int&, int>>, "pair<int&, int> is copy assignable");
		static_assert(!eastl::is_copy_assignable_v<pair<const int&, int>>, "pair<const int&, int> is not copy assignable");

		int a = 1, b = 2;
		pair<int&, int> p1(a, 3);
		const pair<int&, int> p2(b, 4);
		p1 = p2;
		EATEST_VERIFY((a == 2) && (&p1.first == &a) && (p1.second == 4));
	}
#endif

#ifndef EA_COMPILER_NO_STRUCTURED_BINDING
	// pair structured bindings test 
	{
		eastl::pair<int, int> t = {1,2};
		auto [x,y] = t;
		EATEST_VERIFY(x == 1);
		EATEST_VERIFY(y == 2);
	}

	{
		auto t = eastl::make_pair(1, 2);
		auto [x,y] = t;
		EATEST_VERIFY(x == 1);
		EATEST_VERIFY(y == 2);
	}

	{ // reported user-regression structured binding unpacking for iterators
		eastl::vector<int> v = {1,2,3,4,5,6};
		auto t = eastl::make_pair(v.begin(), v.end() - 1);
		auto [x,y] = t;
		EATEST_VERIFY(*x == 1);
		EATEST_VERIFY(*y == 6);
	}

	{ // reported user-regression structured binding unpacking for iterators
		eastl::vector<int> v = {1,2,3,4,5,6};
		auto t = eastl::make_pair(v.begin(), v.end());
		auto [x,y] = t;
		EATEST_VERIFY(*x == 1);
		EA_UNUSED(y);
	}

	{ // reported user-regression for const structured binding unpacking for iterators
		eastl::vector<int> v = {1,2,3,4,5,6};
		const auto [x,y] = eastl::make_pair(v.begin(), v.end());;
		EATEST_VERIFY(*x == 1);
		EA_UNUSED(y);
	}
#endif

	return nErrorCount;
}

///////////////////////////////////////////////////////////////////////////////
// TestUtilityRelops
//
static int TestUtilityRelops()
{
	int nErrorCount = 0;

	{
		using namespace eastl::rel_ops;  // Defines default versions of operators !=, <, >, <=, >= based on == and <.

		BasicObject bo1(1), bo2(2);

		EATEST_VERIFY(!(bo1 == bo2));
		EATEST_VERIFY((bo1 != bo2));
		EATEST_VERIFY((bo1 < bo2));
		EATEST_VERIFY(!(bo1 > bo2));
		EATEST_VERIFY((bo1 <= bo2));
		EATEST_VERIFY(!(bo1 >= bo2));
	}

	return nErrorCount;
}

// ThrowSwappable
struct ThrowSwappable
{
};

void swap(ThrowSwappable& x, ThrowSwappable& y) EA_NOEXCEPT_IF(false)
{
	ThrowSwappable temp(x);
	x = y;
	y = temp;

#if EASTL_EXCEPTIONS_ENABLED
	throw int();
#endif
}

#if EASTL_TYPE_TRAIT_is_nothrow_swappable_CONFORMANCE
// NoThrowSwappable
struct NoThrowSwappable
{
};

void swap(NoThrowSwappable& x, NoThrowSwappable& y) EA_NOEXCEPT_IF(true)
{
	NoThrowSwappable temp(x);
	x = y;
	y = temp;
}
#endif

struct Swappable1 {};
struct Swappable2 {};
struct Swappable3 {};
void swap(Swappable1&, Swappable2&) {} 
void swap(Swappable2&, Swappable1&) {} 
void swap(Swappable1&, Swappable3&) {} // intentionally missing 'swap(Swappable3, Swappable1)'

namespace SwapTest
{
	// Neither movable nor swappable.
	struct NotMovable
	{
		NotMovable() = default;
		NotMovable(NotMovable&&) = delete;
		NotMovable& operator=(NotMovable&&) = delete;
	};

	// Not movable, but swappable through argument-dependent lookup.
	struct AdlSwappable
	{
		int mValue = 0;

		AdlSwappable() = default;
		AdlSwappable(AdlSwappable&&) = delete;
		AdlSwappable& operator=(AdlSwappable&&) = delete;
	};

	inline void swap(AdlSwappable& a, AdlSwappable& b) { eastl::swap(a.mValue, b.mValue); }

	// Two equally good swap overloads: swapping is ambiguous.
	struct AmbiguousSwap {};

	template <typename T> void swap(T&, AmbiguousSwap&) {}
	template <typename T> void swap(AmbiguousSwap&, T&) {}

	// Function objects that can't be assigned, only swapped through argument-dependent lookup.
	struct AdlSwappableLess
	{
		int mId = 0;

		AdlSwappableLess() = default;
		explicit AdlSwappableLess(int id) : mId(id) {}
		AdlSwappableLess(const AdlSwappableLess&) = default;
		AdlSwappableLess& operator=(const AdlSwappableLess&) = delete;

		bool operator()(int a, int b) const { return a < b; }
	};

	inline void swap(AdlSwappableLess& a, AdlSwappableLess& b) { eastl::swap(a.mId, b.mId); }

	struct AdlSwappableHash
	{
		int mId = 0;

		AdlSwappableHash() = default;
		explicit AdlSwappableHash(int id) : mId(id) {}
		AdlSwappableHash(const AdlSwappableHash&) = default;
		AdlSwappableHash& operator=(const AdlSwappableHash&) = delete;

		size_t operator()(int a) const { return (size_t)a; }
	};

	inline void swap(AdlSwappableHash& a, AdlSwappableHash& b) { eastl::swap(a.mId, b.mId); }
}


static int TestUtilitySwap()
{
	int nErrorCount = 0;

// is_swappable
// is_nothrow_swappable
#if EASTL_TYPE_TRAIT_is_swappable_CONFORMANCE
	static_assert((eastl::is_swappable<int>::value == true), "is_swappable failure");
	static_assert((eastl::is_swappable<eastl::vector<int> >::value == true), "is_swappable failure");
	static_assert((eastl::is_swappable<ThrowSwappable>::value == true), "is_swappable failure");
	#if EASTL_VARIABLE_TEMPLATES_ENABLED
		static_assert((eastl::is_swappable_v<int> == true), "is_swappable failure");
		static_assert((eastl::is_swappable_v<eastl::vector<int> > == true), "is_swappable failure");
		static_assert((eastl::is_swappable_v<ThrowSwappable> == true), "is_swappable failure");
	#endif
	// The generic and array swaps require movable / swappable types; an ambiguous swap gives false, not an error.
	static_assert(!eastl::is_swappable<SwapTest::NotMovable>::value, "is_swappable failure");
	static_assert(eastl::is_swappable<SwapTest::AdlSwappable>::value, "is_swappable failure");
	static_assert(!eastl::is_swappable<SwapTest::AmbiguousSwap>::value, "is_swappable failure");
	static_assert(eastl::is_swappable<int[3]>::value, "is_swappable failure");
	static_assert(!eastl::is_swappable<SwapTest::NotMovable[2]>::value, "is_swappable failure");
	static_assert(eastl::is_swappable<SwapTest::AdlSwappable[2]>::value, "is_swappable failure");
	static_assert(!eastl::is_swappable<void>::value, "is_swappable failure");

	// pair, optional and variant swaps require swappable (and, for optional and variant, movable) elements.
	static_assert(!eastl::is_swappable<eastl::pair<SwapTest::NotMovable, int>>::value, "is_swappable failure");
	static_assert(eastl::is_swappable<eastl::pair<SwapTest::AdlSwappable, int>>::value, "is_swappable failure");
	static_assert(!eastl::is_swappable<eastl::optional<SwapTest::NotMovable>>::value, "is_swappable failure");
	static_assert(!eastl::is_swappable<eastl::variant<SwapTest::NotMovable>>::value, "is_swappable failure");
#endif

#if EASTL_TYPE_TRAIT_is_nothrow_swappable_CONFORMANCE
	static_assert((eastl::is_nothrow_swappable<int>::value == true), "is_nothrow_swappable failure");  // There currently isn't any specialization for swap of scalar types that's nothrow.
	static_assert((eastl::is_nothrow_swappable<eastl::vector<int> >::value == false), "is_nothrow_swappable failure");
	static_assert((eastl::is_nothrow_swappable<ThrowSwappable>::value == false), "is_nothrow_swappable failure");
	static_assert((eastl::is_nothrow_swappable<NoThrowSwappable>::value == true), "is_nothrow_swappable failure");
	#if EASTL_VARIABLE_TEMPLATES_ENABLED
		static_assert((eastl::is_nothrow_swappable_v<int> == true), "is_nothrow_swappable failure");  // There currently isn't any specialization for swap of scalar types that's nothrow.
		static_assert((eastl::is_nothrow_swappable_v<eastl::vector<int>> == false), "is_nothrow_swappable failure");
		static_assert((eastl::is_nothrow_swappable_v<ThrowSwappable> == false), "is_nothrow_swappable failure");
		static_assert((eastl::is_nothrow_swappable_v<NoThrowSwappable> == true), "is_nothrow_swappable failure");
	#endif
#endif

#if EASTL_VARIADIC_TEMPLATES_ENABLED
// is_swappable_with
// is_nothrow_swappable_with
	static_assert(eastl::is_swappable_with<int&, int&>::value, "is_swappable_with failure");
	static_assert(!eastl::is_swappable_with<int, int>::value, "is_swappable_with failure");
	static_assert(!eastl::is_swappable_with<int&, int>::value, "is_swappable_with failure");
	static_assert(!eastl::is_swappable_with<int, int&>::value, "is_swappable_with failure");
	static_assert(!eastl::is_swappable_with<int, short>::value, "is_swappable_with failure");
	static_assert(!eastl::is_swappable_with<int, long>::value, "is_swappable_with failure");
	static_assert(!eastl::is_swappable_with<int, eastl::vector<int>>::value, "is_swappable_with failure");
	static_assert(!eastl::is_swappable_with<void, void>::value, "is_swappable_with failure");
	static_assert(!eastl::is_swappable_with<int, void>::value, "is_swappable_with failure");
	static_assert(!eastl::is_swappable_with<void, int>::value, "is_swappable_with failure");
	static_assert(!eastl::is_swappable_with<ThrowSwappable, ThrowSwappable>::value, "is_swappable_with failure");
	static_assert(eastl::is_swappable_with<ThrowSwappable&, ThrowSwappable&>::value, "is_swappable_with failure");
	static_assert(eastl::is_swappable_with<Swappable1&, Swappable1&>::value, "is_swappable_with failure");
	static_assert(eastl::is_swappable_with<Swappable1&, Swappable2&>::value, "is_swappable_with failure");
	static_assert(eastl::is_swappable_with<Swappable2&, Swappable1&>::value, "is_swappable_with failure");

    #if EASTL_VARIABLE_TEMPLATES_ENABLED
		static_assert(eastl::is_swappable_with_v<int&, int&>, "is_swappable_with_v failure");
		static_assert(!eastl::is_swappable_with_v<int, int>, "is_swappable_with_v failure");
		static_assert(!eastl::is_swappable_with_v<int&, int>, "is_swappable_with_v failure");
		static_assert(!eastl::is_swappable_with_v<int, int&>, "is_swappable_with_v failure");
		static_assert(!eastl::is_swappable_with_v<int, short>, "is_swappable_with_v failure");
		static_assert(!eastl::is_swappable_with_v<int, long>, "is_swappable_with_v failure");
		static_assert(!eastl::is_swappable_with_v<int, eastl::vector<int>>, "is_swappable_with_v failure");
	    static_assert(!eastl::is_swappable_with_v<void, void>, "is_swappable_with_v failure");
	    static_assert(!eastl::is_swappable_with_v<int, void>, "is_swappable_with_v failure");
	    static_assert(!eastl::is_swappable_with_v<void, int>, "is_swappable_with_v failure");
		static_assert(!eastl::is_swappable_with_v<ThrowSwappable, ThrowSwappable>, "is_swappable_with_v failure");
		static_assert(eastl::is_swappable_with_v<ThrowSwappable&, ThrowSwappable&>, "is_swappable_with_v failure");
		static_assert(eastl::is_swappable_with_v<Swappable1&, Swappable1&>, "is_swappable_with_v failure");
		static_assert(eastl::is_swappable_with_v<Swappable1&, Swappable2&>, "is_swappable_with_v failure");
		static_assert(eastl::is_swappable_with_v<Swappable2&, Swappable1&>, "is_swappable_with_v failure");
    #endif // EASTL_VARIABLE_TEMPLATES_ENABLED

#if EASTL_TYPE_TRAIT_is_nothrow_swappable_with_CONFORMANCE
	static_assert(eastl::is_nothrow_swappable_with<int&, int&>::value, "is_nothrow_swappable_with failure");
	static_assert(!eastl::is_nothrow_swappable_with<int, int>::value, "is_nothrow_swappable_with failure");
	static_assert(!eastl::is_nothrow_swappable_with<int&, int>::value, "is_nothrow_swappable_with failure");
	static_assert(!eastl::is_nothrow_swappable_with<int, int&>::value, "is_nothrow_swappable_with failure");
	static_assert(!eastl::is_nothrow_swappable_with<int, short>::value, "is_nothrow_swappable_with failure");
	static_assert(!eastl::is_nothrow_swappable_with<int, long>::value, "is_nothrow_swappable_with failure");
	static_assert(!eastl::is_nothrow_swappable_with<int, eastl::vector<int>>::value, "is_nothrow_swappable_with failure");
	static_assert(!eastl::is_nothrow_swappable_with<void, void>::value, "is_nothrow_swappable_with failure");
	static_assert(!eastl::is_nothrow_swappable_with<int, void>::value, "is_nothrow_swappable_with failure");
	static_assert(!eastl::is_nothrow_swappable_with<void, int>::value, "is_nothrow_swappable_with failure");
	static_assert(!eastl::is_nothrow_swappable_with<ThrowSwappable, ThrowSwappable>::value, "is_nothrow_swappable_with failure");
	static_assert(!eastl::is_nothrow_swappable_with<ThrowSwappable&, ThrowSwappable&>::value, "is_nothrow_swappable_with failure");
	static_assert(!eastl::is_nothrow_swappable_with<NoThrowSwappable, NoThrowSwappable>::value, "is_nothrow_swappable_with failure");
	static_assert(eastl::is_nothrow_swappable_with<NoThrowSwappable&, NoThrowSwappable&>::value, "is_nothrow_swappable_with failure");

    #if EASTL_VARIABLE_TEMPLATES_ENABLED
		static_assert(eastl::is_nothrow_swappable_with_v<int&, int&>, "is_nothrow_swappable_with_v failure");
		static_assert(!eastl::is_nothrow_swappable_with_v<int, int>, "is_nothrow_swappable_with_v failure");
		static_assert(!eastl::is_nothrow_swappable_with_v<int&, int>, "is_nothrow_swappable_with_v failure");
		static_assert(!eastl::is_nothrow_swappable_with_v<int, int&>, "is_nothrow_swappable_with_v failure");
		static_assert(!eastl::is_nothrow_swappable_with_v<int, short>, "is_nothrow_swappable_with_v failure");
		static_assert(!eastl::is_nothrow_swappable_with_v<int, long>, "is_nothrow_swappable_with_v failure");
		static_assert(!eastl::is_nothrow_swappable_with_v<int, eastl::vector<int>>, "is_nothrow_swappable_with_v failure");
	    static_assert(!eastl::is_nothrow_swappable_with_v<void, void>, "is_nothrow_swappable_with_v failure");
	    static_assert(!eastl::is_nothrow_swappable_with_v<int, void>, "is_nothrow_swappable_with_v failure");
	    static_assert(!eastl::is_nothrow_swappable_with_v<void, int>, "is_nothrow_swappable_with_v failure");
		static_assert(!eastl::is_nothrow_swappable_with_v<ThrowSwappable, ThrowSwappable>, "is_nothrow_swappable_with_v failure");
		static_assert(!eastl::is_nothrow_swappable_with_v<ThrowSwappable&, ThrowSwappable&>, "is_nothrow_swappable_with_v failure");
		static_assert(!eastl::is_nothrow_swappable_with_v<NoThrowSwappable, NoThrowSwappable>, "is_nothrow_swappable_with_v failure");
		static_assert(eastl::is_nothrow_swappable_with_v<NoThrowSwappable&, NoThrowSwappable&>, "is_nothrow_swappable_with_v failure");
    #endif // EASTL_VARIABLE_TEMPLATES_ENABLED
#endif
#endif // EASTL_VARIADIC_TEMPLATES_ENABLED

	{
		// User-defined swaps found via argument-dependent lookup are used.
		SwapTest::AdlSwappable a[2], b[2];
		a[0].mValue = 1;
		b[0].mValue = 2;
		eastl::swap(a, b);
		EATEST_VERIFY(a[0].mValue == 2 && b[0].mValue == 1);

		eastl::pair<SwapTest::AdlSwappable, int> p1, p2;
		p1.first.mValue = 1; p1.second = 1;
		p2.first.mValue = 2; p2.second = 2;
		swap(p1, p2);
		EATEST_VERIFY(p1.first.mValue == 2 && p1.second == 2 && p2.first.mValue == 1 && p2.second == 1);

		eastl::tuple<SwapTest::AdlSwappable> t1, t2;
		eastl::get<0>(t1).mValue = 1;
		eastl::get<0>(t2).mValue = 2;
		t1.swap(t2);
		EATEST_VERIFY(eastl::get<0>(t1).mValue == 2 && eastl::get<0>(t2).mValue == 1);

		// Containers swap their comparators and hash functions via argument-dependent lookup.
		eastl::map<int, int, SwapTest::AdlSwappableLess> m1(SwapTest::AdlSwappableLess(1)), m2(SwapTest::AdlSwappableLess(2));
		m1[1] = 1;
		m1.swap(m2);
		EATEST_VERIFY(m1.key_comp().mId == 2 && m2.key_comp().mId == 1 && m1.empty() && m2.size() == 1);

		eastl::vector_map<int, int, SwapTest::AdlSwappableLess> vm1(SwapTest::AdlSwappableLess(1)), vm2(SwapTest::AdlSwappableLess(2));
		vm1[1] = 1;
		vm1.swap(vm2);
		EATEST_VERIFY(vm1.key_comp().mId == 2 && vm2.key_comp().mId == 1 && vm1.empty() && vm2.size() == 1);

		eastl::hash_map<int, int, SwapTest::AdlSwappableHash> h1(4, SwapTest::AdlSwappableHash(1)), h2(4, SwapTest::AdlSwappableHash(2));
		h1[1] = 1;
		h1.swap(h2);
		EATEST_VERIFY(h1.hash_function().mId == 2 && h2.hash_function().mId == 1 && h1.empty() && h2.size() == 1);
	}

#ifndef EA_COMPILER_NO_STANDARD_CPP_LIBRARY
	{
		// std code on EASTL types: std::swap and eastl::swap are both found by argument-dependent lookup
		// and must not be ambiguous.
		eastl::pair<int, int> x(1, 1), y(2, 2);

		eastl::pair<int, int>* ptrs[2] = { &x, &y };
		std::reverse(ptrs, ptrs + 2);
		EATEST_VERIFY(ptrs[0] == &y && ptrs[1] == &x);

		std::vector<eastl::pair<int, int>*> ptrVector = { &y, &x };
		std::sort(ptrVector.begin(), ptrVector.end(), [](eastl::pair<int, int>* a, eastl::pair<int, int>* b) { return a->first < b->first; });
		EATEST_VERIFY(ptrVector[0] == &x && ptrVector[1] == &y);

		using std::swap;
		swap(x, y);
		EATEST_VERIFY(x.first == 2 && y.first == 1);

		eastl::pair<int, int> xa[2], ya[2];
		xa[0].first = 1;
		ya[0].first = 2;
		swap(xa, ya);
		EATEST_VERIFY(xa[0].first == 2 && ya[0].first == 1);

		static_assert(std::is_swappable<eastl::pair<int, int>>::value, "std::is_swappable failure");
		static_assert(std::is_swappable<eastl::pair<int, int>*>::value, "std::is_swappable failure");
	}

	{
		// EASTL code on std types.
		static_assert(eastl::is_swappable<std::chrono::seconds>::value, "is_swappable failure");

		std::chrono::seconds s1(1), s2(2);
		using eastl::swap;
		swap(s1, s2);
		EATEST_VERIFY(s1.count() == 2 && s2.count() == 1);

		eastl::vector<std::chrono::seconds> v;
		v.push_back(std::chrono::seconds(3));
		v.push_back(std::chrono::seconds(1));
		v.push_back(std::chrono::seconds(2));
		eastl::sort(v.begin(), v.end());
		EATEST_VERIFY(v[0].count() == 1 && v[1].count() == 2 && v[2].count() == 3);
		eastl::reverse(v.begin(), v.end());
		EATEST_VERIFY(v[0].count() == 3 && v[1].count() == 2 && v[2].count() == 1);

		eastl::optional<std::chrono::seconds> o1(std::chrono::seconds(5)), o2;
		o1.swap(o2);
		EATEST_VERIFY(!o1 && o2 && o2->count() == 5);

		eastl::vector_map<int, int, std::less<int>> vm1, vm2;
		vm1[1] = 1;
		vm1.swap(vm2);
		EATEST_VERIFY(vm1.empty() && vm2.size() == 1);
	}
#endif

	return nErrorCount;
}

#if !defined(EA_COMPILER_NO_NOEXCEPT)

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Warning C4626 warns against an implicitly deleted move assignment operator.
// This warning was disabled by default in VS2013. It was enabled by default in
// VS2015.  Since the the tests below are explicitly testing move construction
// of the various classes explicitly deleting the move assignment to remove the
// warning is safe. 
//
// https://msdn.microsoft.com/en-us/library/23k5d385.aspx

struct noexcept_move_copy
{
	bool mStatus;

	noexcept_move_copy() : mStatus(true) {}

	noexcept_move_copy(const noexcept_move_copy&) = default;

	noexcept_move_copy(noexcept_move_copy&& r) noexcept { r.mStatus = false; }

	noexcept_move_copy& operator=(const noexcept_move_copy&) = delete;  // required as VS2015 enabled C4626 by default.
};

struct noexcept_move_no_copy
{
	bool mStatus;

	noexcept_move_no_copy() : mStatus(true) {}

	noexcept_move_no_copy(const noexcept_move_no_copy&) = delete;

	noexcept_move_no_copy(noexcept_move_no_copy&& r) noexcept { r.mStatus = false; };

	noexcept_move_no_copy& operator=(const noexcept_move_no_copy&) = delete;  // required as VS2015 enabled C4626 by default.
};

struct except_move_copy
{
	bool mStatus;

	except_move_copy() : mStatus(true) {}

	except_move_copy(const except_move_copy&) = default;

	except_move_copy(except_move_copy&& r) noexcept(false) { r.mStatus = false; };

	except_move_copy& operator=(const except_move_copy&) = delete;  // required as VS2015 enabled C4626 by default.
};

struct except_move_no_copy
{
	bool mStatus;

	except_move_no_copy() : mStatus(true) {}

	except_move_no_copy(const except_move_no_copy&) = delete;

	except_move_no_copy(except_move_no_copy&& r) noexcept(false) { r.mStatus = false; };

	except_move_no_copy& operator=(const except_move_no_copy&) = delete;  // required as VS2015 enabled C4626 by default.
};
#endif

static int TestUtilityMove()
{
	int nErrorCount = 0;

// move_if_noexcept
#if !defined(EA_COMPILER_NO_NOEXCEPT)
	noexcept_move_copy nemcA;
	noexcept_move_copy nemcB =
		eastl::move_if_noexcept(nemcA);  // nemcB should be constructed via noexcept_move_copy(noexcept_move_copy&&)
	EATEST_VERIFY(nemcA.mStatus == false);
	EA_UNUSED(nemcB);

	noexcept_move_no_copy nemncA;
	noexcept_move_no_copy nemncB = eastl::move_if_noexcept(
		nemncA);  // nemncB should be constructed via noexcept_move_no_copy(noexcept_move_no_copy&&)
	EATEST_VERIFY(nemncA.mStatus == false);
	EA_UNUSED(nemncB);

	except_move_copy emcA;
	except_move_copy emcB = eastl::move_if_noexcept(
		emcA);  // emcB should be constructed via except_move_copy(const except_move_copy&) if exceptions are enabled.
#if EASTL_EXCEPTIONS_ENABLED
	EATEST_VERIFY(emcA.mStatus == true);
#else
	EATEST_VERIFY(emcA.mStatus == false);
#endif
	EA_UNUSED(emcB);

	except_move_no_copy emncA;
	except_move_no_copy emncB =
		eastl::move_if_noexcept(emncA);  // emncB should be constructed via except_move_no_copy(except_move_no_copy&&)
	EATEST_VERIFY(emncA.mStatus == false);
	EA_UNUSED(emncB);
#endif

	return nErrorCount;
}

static int TestUtilityIntegerSequence()
{
	using namespace eastl;
	int nErrorCount = 0;
#if EASTL_VARIADIC_TEMPLATES_ENABLED
	
	EATEST_VERIFY((integer_sequence<int, 0, 1, 2, 3, 4>::size() == 5));
	EATEST_VERIFY((make_integer_sequence<int, 5>::size() == 5));
	static_assert(is_same<make_integer_sequence<int, 5>, integer_sequence<int, 0, 1, 2, 3, 4>>::value);

	EATEST_VERIFY((index_sequence<0, 1, 2, 3, 4>::size() == 5));
	EATEST_VERIFY((make_index_sequence<5>::size() == 5));
	static_assert(is_same<make_index_sequence<5>, index_sequence<0, 1, 2, 3, 4>>::value);
	static_assert(is_same<make_index_sequence<5>, integer_sequence<size_t, 0, 1, 2, 3, 4>>::value);
#endif  // EASTL_VARIADIC_TEMPLATES_ENABLED

	return nErrorCount;
}

static int TestUtilityExchange()
{
	int nErrorCount = 0;

	{
		int a = 0;
		auto r = eastl::exchange(a, 1);

		EATEST_VERIFY(r == 0);
		EATEST_VERIFY(a == 1);
	}

	{
		int a = 0;
		auto r = eastl::exchange(a, 1.78);

		EATEST_VERIFY(r == 0);
		EATEST_VERIFY(a == 1);
	}

	{
		int a = 0;
		auto r = eastl::exchange(a, 1.78f);

		EATEST_VERIFY(r == 0);
		EATEST_VERIFY(a == 1);
	}

	{
		int a = 0, b = 1;
		auto r = eastl::exchange(a, b);

		EATEST_VERIFY(r == 0);
		EATEST_VERIFY(a == 1);
		EATEST_VERIFY(b == 1);
	}

	{
		bool b = true;

		auto r = eastl::exchange(b, true);
		EATEST_VERIFY(r);

		r = eastl::exchange(b, false);
		EATEST_VERIFY(r);
		EATEST_VERIFY(!b);

		r = eastl::exchange(b, true);
		EATEST_VERIFY(!r);
		EATEST_VERIFY(b);
	}

	{
		TestObject::Reset();

		TestObject a(42);
		auto r = eastl::exchange(a, TestObject(24));

		EATEST_VERIFY(r.mX == 42);
		EATEST_VERIFY(a.mX == 24);
	}

	{
		const char* const pElectronicArts = "Electronic Arts";
		const char* const pEAVancouver = "EA Vancouver";

		eastl::string a(pElectronicArts);
		auto r = eastl::exchange(a, pEAVancouver);

		EATEST_VERIFY(r == pElectronicArts);
		EATEST_VERIFY(a == pEAVancouver);

		r = eastl::exchange(a, "EA Standard Template Library");
		EATEST_VERIFY(a == "EA Standard Template Library");
	}

	// Construct pair using single move constructor
	{
		struct TestPairSingleMoveConstructor
		{
			void test(int&& val)
			{
				eastl::pair<int,int> p(eastl::pair_first_construct, eastl::move(val));
			}
		};

		int i1 = 1;
		TestPairSingleMoveConstructor test;
		test.test(eastl::move(i1));
	}

	// User reported regression where via reference collapsing, we see the same single element ctor defined twice.
	//
	// T = const U&
	// pair(const T&) -> pair(const const U& &) -> pair(const U&)
	// pair(T&&)      -> pair(const U& &&)      -> pair(const U&)
	{
		struct FooType {};

		using VectorOfPairWithReference = eastl::vector<eastl::pair<const FooType&, float>>;

		VectorOfPairWithReference v;
	}

	return nErrorCount;
}

#if EASTL_INLINE_VARIABLE_ENABLED && EASTL_TUPLE_ENABLED && EASTL_OPTIONAL_ENABLED
// Defined in TestTuple.cpp and TestOptional.cpp: the addresses of the tag objects in those translation units.
const void* TestTupleIgnoreAddress();
const void* TestTuplePiecewiseConstructAddress();
const void* TestOptionalNulloptAddress();

static int TestUtilityTagObjects()
{
	int nErrorCount = 0;

	// Like the std ones, the tag objects are inline variables: every translation unit sees the same object.
	EATEST_VERIFY(TestTupleIgnoreAddress() == &eastl::ignore);
	EATEST_VERIFY(TestTuplePiecewiseConstructAddress() == &eastl::piecewise_construct);
	EATEST_VERIFY(TestOptionalNulloptAddress() == &eastl::nullopt);

	return nErrorCount;
}
#endif

#if defined(EA_COMPILER_CPP17_ENABLED)
template <typename T>
static int TestCmpCommon()
{
	int nErrorCount = 0;

	EATEST_VERIFY(eastl::cmp_equal(T(0), T(0)));
	EATEST_VERIFY(eastl::cmp_equal(T(1), T(1)));
	EATEST_VERIFY(eastl::cmp_equal(eastl::numeric_limits<T>::min(), eastl::numeric_limits<T>::min()));
	EATEST_VERIFY(eastl::cmp_equal(eastl::numeric_limits<T>::max(), eastl::numeric_limits<T>::max()));
	EATEST_VERIFY(!eastl::cmp_equal(T(0), T(1)));
	EATEST_VERIFY(!eastl::cmp_equal(T(1), T(0)));
	if (eastl::is_signed_v<T>)
	{
		EATEST_VERIFY(eastl::cmp_equal(T(-1), T(-1)));
		EATEST_VERIFY(!eastl::cmp_equal(T(-1), T(-2)));
		EATEST_VERIFY(!eastl::cmp_equal(T(-2), T(-1)));
	}

	EATEST_VERIFY(eastl::cmp_not_equal(T(1), T(0)));
	EATEST_VERIFY(eastl::cmp_not_equal(T(0), T(1)));
	EATEST_VERIFY(eastl::cmp_not_equal(eastl::numeric_limits<T>::min(), eastl::numeric_limits<T>::max()));
	EATEST_VERIFY(eastl::cmp_not_equal(eastl::numeric_limits<T>::max(), eastl::numeric_limits<T>::min()));
	if (eastl::is_signed_v<T>)
	{
		EATEST_VERIFY(!eastl::cmp_not_equal(T(-1), T(-1)));
		EATEST_VERIFY(eastl::cmp_not_equal(T(-1), T(-2)));
		EATEST_VERIFY(eastl::cmp_not_equal(T(-2), T(-1)));
	}

	EATEST_VERIFY(eastl::cmp_less(T(0), T(1)));
	EATEST_VERIFY(eastl::cmp_less(T(5), T(10)));
	EATEST_VERIFY(!eastl::cmp_less(T(0), T(0)));
	EATEST_VERIFY(!eastl::cmp_less(T(1), T(0)));
	EATEST_VERIFY(eastl::cmp_less(eastl::numeric_limits<T>::min(), eastl::numeric_limits<T>::max()));
	EATEST_VERIFY(!eastl::cmp_less(eastl::numeric_limits<T>::min(), eastl::numeric_limits<T>::min()));
	EATEST_VERIFY(!eastl::cmp_less(eastl::numeric_limits<T>::max(), eastl::numeric_limits<T>::max()));
	EATEST_VERIFY(!eastl::cmp_less(eastl::numeric_limits<T>::max(), eastl::numeric_limits<T>::min()));
	if (eastl::is_signed_v<T>)
	{
		EATEST_VERIFY(!eastl::cmp_less(T(-1), T(-1)));
		EATEST_VERIFY(!eastl::cmp_less(T(-1), T(-2)));
		EATEST_VERIFY(eastl::cmp_less(T(-2), T(-1)));
	}

	EATEST_VERIFY(eastl::cmp_less_equal(T(0), T(1)));
	EATEST_VERIFY(eastl::cmp_less_equal(T(5), T(10)));
	EATEST_VERIFY(eastl::cmp_less_equal(T(0), T(0)));
	EATEST_VERIFY(eastl::cmp_less_equal(T(1), T(1)));
	EATEST_VERIFY(!eastl::cmp_less_equal(T(1), T(0)));
	EATEST_VERIFY(eastl::cmp_less_equal(eastl::numeric_limits<T>::min(), eastl::numeric_limits<T>::max()));
	EATEST_VERIFY(eastl::cmp_less_equal(eastl::numeric_limits<T>::min(), eastl::numeric_limits<T>::min()));
	EATEST_VERIFY(eastl::cmp_less_equal(eastl::numeric_limits<T>::max(), eastl::numeric_limits<T>::max()));
	EATEST_VERIFY(!eastl::cmp_less_equal(eastl::numeric_limits<T>::max(), eastl::numeric_limits<T>::min()));
	if (eastl::is_signed_v<T>)
	{
		EATEST_VERIFY(eastl::cmp_less_equal(T(-1), T(-1)));
		EATEST_VERIFY(!eastl::cmp_less_equal(T(-1), T(-2)));
		EATEST_VERIFY(eastl::cmp_less_equal(T(-2), T(-1)));
	}

	EATEST_VERIFY(eastl::cmp_greater(T(1), T(0)));
	EATEST_VERIFY(eastl::cmp_greater(T(10), T(5)));
	EATEST_VERIFY(!eastl::cmp_greater(T(0), T(0)));
	EATEST_VERIFY(!eastl::cmp_greater(T(0), T(1)));
	EATEST_VERIFY(eastl::cmp_greater(eastl::numeric_limits<T>::max(), eastl::numeric_limits<T>::min()));
	EATEST_VERIFY(!eastl::cmp_greater(eastl::numeric_limits<T>::min(), eastl::numeric_limits<T>::min()));
	EATEST_VERIFY(!eastl::cmp_greater(eastl::numeric_limits<T>::max(), eastl::numeric_limits<T>::max()));
	EATEST_VERIFY(!eastl::cmp_greater(eastl::numeric_limits<T>::min(), eastl::numeric_limits<T>::max()));
	if (eastl::is_signed_v<T>)
	{
		EATEST_VERIFY(!eastl::cmp_greater(T(-1), T(-1)));
		EATEST_VERIFY(eastl::cmp_greater(T(-1), T(-2)));
		EATEST_VERIFY(!eastl::cmp_greater(T(-2), T(-1)));
	}

	EATEST_VERIFY(eastl::cmp_greater_equal(T(1), T(0)));
	EATEST_VERIFY(eastl::cmp_greater_equal(T(10), T(5)));
	EATEST_VERIFY(eastl::cmp_greater_equal(T(0), T(0)));
	EATEST_VERIFY(!eastl::cmp_greater_equal(T(0), T(1)));
	EATEST_VERIFY(eastl::cmp_greater_equal(eastl::numeric_limits<T>::max(), eastl::numeric_limits<T>::min()));
	EATEST_VERIFY(eastl::cmp_greater_equal(eastl::numeric_limits<T>::min(), eastl::numeric_limits<T>::min()));
	EATEST_VERIFY(eastl::cmp_greater_equal(eastl::numeric_limits<T>::max(), eastl::numeric_limits<T>::max()));
	EATEST_VERIFY(!eastl::cmp_greater_equal(eastl::numeric_limits<T>::min(), eastl::numeric_limits<T>::max()));
	if (eastl::is_signed_v<T>)
	{
		EATEST_VERIFY(eastl::cmp_greater_equal(T(-1), T(-1)));
		EATEST_VERIFY(eastl::cmp_greater_equal(T(-1), T(-2)));
		EATEST_VERIFY(!eastl::cmp_greater_equal(T(-2), T(-1)));
	}

	return nErrorCount;
}

template <typename T, typename U>
static int TestUtilityCmpEql(const T x, const U y)
{
	int nErrorCount = 0;

	EATEST_VERIFY(eastl::cmp_equal(T(x), U(y)));
	EATEST_VERIFY(eastl::cmp_equal(U(y), T(x)));
	EATEST_VERIFY(!eastl::cmp_not_equal(T(x), U(y)));
	EATEST_VERIFY(!eastl::cmp_not_equal(U(y), T(x)));

	return nErrorCount;
}

template <typename T, typename U>
static int TestUtilityCmpLess(const T x, const U y)
{
	int nErrorCount = 0;

	EATEST_VERIFY(eastl::cmp_less(T(x), U(y)));
	EATEST_VERIFY(!eastl::cmp_less(U(y), T(x)));

	EATEST_VERIFY(!eastl::cmp_greater_equal(T(x), U(y)));
	EATEST_VERIFY(eastl::cmp_greater_equal(U(y), T(x)));

	return nErrorCount;
}

template <typename T, typename U>
static int TestUtilityCmpGreater(const T x, const U y)
{
	int nErrorCount = 0;

	EATEST_VERIFY(eastl::cmp_greater(T(x), U(y)));
	EATEST_VERIFY(!eastl::cmp_greater(U(y), T(x)));

	EATEST_VERIFY(!eastl::cmp_less_equal(T(x), U(y)));
	EATEST_VERIFY(eastl::cmp_less_equal(U(y), T(x)));

	return nErrorCount;
}

template <typename T, typename U>
static int TestUtilityCmpLessEq(const T x, const U y)
{
	int nErrorCount = 0;

	EATEST_VERIFY(eastl::cmp_less_equal(T(x), U(y)));
	EATEST_VERIFY(eastl::cmp_less(T(x), U(y)) || eastl::cmp_equal(T(x), U(y)));

	EATEST_VERIFY(eastl::cmp_greater_equal(U(y), T(x)));

	return nErrorCount;
}

template <typename T, typename U>
static int TestUtilityCmpGreaterEq(const T x, const U y)
{
	int nErrorCount = 0;

	EATEST_VERIFY(eastl::cmp_greater_equal(T(x), U(y)));
	EATEST_VERIFY(eastl::cmp_greater(T(x), U(y)) || eastl::cmp_equal(T(x), U(y)));

	EATEST_VERIFY(eastl::cmp_less_equal(U(y), T(x)));

	return nErrorCount;
}

static int TestUtilityIntegralComp()
{
	int nErrorCount = 0;

	// Test integral comparisons among same types
	nErrorCount += TestCmpCommon<int>();
	nErrorCount += TestCmpCommon<short>();
	nErrorCount += TestCmpCommon<long>();
	nErrorCount += TestCmpCommon<long long>();

	nErrorCount += TestCmpCommon<unsigned int>();
	nErrorCount += TestCmpCommon<unsigned short>();
	nErrorCount += TestCmpCommon<unsigned long>();
	nErrorCount += TestCmpCommon<unsigned long long>();

	//
	// Test integral comparison among different types
	//
	// we're now using integer-suffixes because Clang on
	// unix was failing to construct integer types with a
	// space in them, like 'unsigned long', e.g.
	// 
	//    TestUilityCmpLess(4, unsigned long(4))
	// 
	// 
	// Integer literal suffixes:
	// 
	//    u/U   : unsigned (`unsigned int` by default)
	// 
	//    l/L   : long
	//    ll/LL : long long
	//    z/Z   : (c++23) the signed version of size_t
	//    uz/UZ : (c++23) size_t
	//
	// unsignedness & the width suffixes can be combined together.
	// thus `2ull` is an unsigned long long (of value 2). because
	// there's no ordering for u vs ll, we can also write the
	// equivalent `2llu`, or `2ULL`
	// 
	// NOTE: integer literals that are too large to fit into the
	//       requested type will be promoted a little bit to a
	//       "sensible" (post c++11) higher category. so the
	//       following:
	// 
	//           235763456234623452345L
	//
	//       is way to large to fit into `L` (long int), and so
	//       and so will be interpreted as a `long long int` literal.
	// 
	// DOUBLE NOTE: the above promotion rules are expanded a bit for
	//              binary, hexadecimal, or octal literals. basically
	//              they can be unsigned sometimes. this is why
	//              compilers give you a warning when you mess this up.
	// 
	// the more you know!!
	// 
	// 
	// Further reading
	//     https://en.cppreference.com/w/cpp/language/integer_literal
	//
	nErrorCount += TestUtilityCmpEql(0, short(0));
	nErrorCount += TestUtilityCmpEql(short(2), 2l);
	nErrorCount += TestUtilityCmpEql(short(3), 3ul);
	nErrorCount += TestUtilityCmpEql(-5, -5ll);
	nErrorCount += TestUtilityCmpEql(short(-100), -100ll);
	nErrorCount += TestUtilityCmpEql(100u, 100l);
	nErrorCount += TestUtilityCmpEql(100ull, 100);

	nErrorCount += TestUtilityCmpLess(0, 1ll);
	nErrorCount += TestUtilityCmpLess(-1, 1ul);
	nErrorCount += TestUtilityCmpLess(short(-100), 100ll);
	nErrorCount += TestUtilityCmpLess(eastl::numeric_limits<long>::min(), short(0));
	nErrorCount += TestUtilityCmpLess(short(0), eastl::numeric_limits<int>::max());
	nErrorCount += TestUtilityCmpLess(eastl::numeric_limits<unsigned short>::min(), eastl::numeric_limits<int>::max());
	nErrorCount += TestUtilityCmpLess(eastl::numeric_limits<short>::max(), eastl::numeric_limits<long>::max());
	nErrorCount += TestUtilityCmpLess(eastl::numeric_limits<int>::max(), eastl::numeric_limits<long long>::max());
	nErrorCount += TestUtilityCmpLess(-100, 0u);
	nErrorCount += TestUtilityCmpLess(eastl::numeric_limits<int>::min(), eastl::numeric_limits<unsigned int>::min());

	nErrorCount += TestUtilityCmpGreater(1, short(0));
	nErrorCount += TestUtilityCmpGreater(1ul, -1);
	nErrorCount += TestUtilityCmpGreater(100ull, short(-100));
	nErrorCount += TestUtilityCmpGreater(short(0), eastl::numeric_limits<short>::min());
	nErrorCount += TestUtilityCmpGreater(eastl::numeric_limits<long>::max(), (unsigned short)5);
	nErrorCount += TestUtilityCmpGreater(eastl::numeric_limits<long>::max(), eastl::numeric_limits<int>::min());
	nErrorCount += TestUtilityCmpGreater(eastl::numeric_limits<int>::max(), eastl::numeric_limits<short>::max());
	nErrorCount += TestUtilityCmpGreater(eastl::numeric_limits<long long>::max(), eastl::numeric_limits<int>::max());
	nErrorCount += TestUtilityCmpGreater(0u, -100);
	nErrorCount += TestUtilityCmpGreater(eastl::numeric_limits<unsigned int>::min(), eastl::numeric_limits<int>::min());

	nErrorCount += TestUtilityCmpLessEq(0, short(1));
	nErrorCount += TestUtilityCmpLessEq(-1, -1ll);
	nErrorCount += TestUtilityCmpLessEq(short(-100), 100ull);
	nErrorCount += TestUtilityCmpLessEq(short(-100), -100ll);
	nErrorCount += TestUtilityCmpLessEq(eastl::numeric_limits<int>::min(), short(0));
	nErrorCount += TestUtilityCmpLessEq(short(0), eastl::numeric_limits<int>::max());
	nErrorCount += TestUtilityCmpLessEq(eastl::numeric_limits<short>::min(), eastl::numeric_limits<short>::min());
	nErrorCount += TestUtilityCmpLessEq(eastl::numeric_limits<int>::max(), eastl::numeric_limits<int>::max());
	nErrorCount += TestUtilityCmpLessEq(eastl::numeric_limits<int>::max(), eastl::numeric_limits<long long>::max());
	nErrorCount += TestUtilityCmpLessEq(50, 50u);
	nErrorCount += TestUtilityCmpLessEq(eastl::numeric_limits<int>::min(), eastl::numeric_limits<unsigned int>::min());

	nErrorCount += TestUtilityCmpGreaterEq(1, short(1));
	nErrorCount += TestUtilityCmpGreaterEq(-1ll, -1);
	nErrorCount += TestUtilityCmpGreaterEq(-100ll, short(-100));
	nErrorCount += TestUtilityCmpGreaterEq(short(0), 0l);
	nErrorCount += TestUtilityCmpGreaterEq(eastl::numeric_limits<long>::max(), eastl::numeric_limits<long>::max());
	nErrorCount += TestUtilityCmpGreaterEq(eastl::numeric_limits<int>::max(), eastl::numeric_limits<short>::min());
	nErrorCount += TestUtilityCmpGreaterEq(eastl::numeric_limits<int>::max(), eastl::numeric_limits<short>::max());
	nErrorCount += TestUtilityCmpGreaterEq(eastl::numeric_limits<long long>::max(), eastl::numeric_limits<int>::max());
	nErrorCount += TestUtilityCmpGreaterEq(0u, 0);
	nErrorCount += TestUtilityCmpGreaterEq(eastl::numeric_limits<unsigned int>::min(), eastl::numeric_limits<int>::min());

	// Test in_range
	EATEST_VERIFY(eastl::in_range<int>(0));
	EATEST_VERIFY(eastl::in_range<int>(eastl::numeric_limits<int>::min()));
	EATEST_VERIFY(eastl::in_range<int>(eastl::numeric_limits<int>::max()));
	EATEST_VERIFY(eastl::in_range<unsigned int>(0));
	EATEST_VERIFY(eastl::in_range<unsigned int>(eastl::numeric_limits<unsigned int>::min()));
	EATEST_VERIFY(eastl::in_range<unsigned int>(eastl::numeric_limits<unsigned int>::max()));
	EATEST_VERIFY(!eastl::in_range<unsigned int>(-1));
	EATEST_VERIFY(!eastl::in_range<int>(eastl::numeric_limits<unsigned int>::max()));
	EATEST_VERIFY(!eastl::in_range<unsigned int>(eastl::numeric_limits<int>::min()));

	EATEST_VERIFY(eastl::in_range<short>(100));
	EATEST_VERIFY(eastl::in_range<short>(eastl::numeric_limits<short>::min()));
	EATEST_VERIFY(eastl::in_range<short>(eastl::numeric_limits<short>::max()));
	EATEST_VERIFY(eastl::in_range<unsigned short>(100));
	EATEST_VERIFY(eastl::in_range<unsigned short>(eastl::numeric_limits<unsigned short>::min()));
	EATEST_VERIFY(eastl::in_range<unsigned short>(eastl::numeric_limits<unsigned short>::max()));
	EATEST_VERIFY(!eastl::in_range<unsigned short>(-1));
	EATEST_VERIFY(!eastl::in_range<short>(eastl::numeric_limits<unsigned int>::max()));
	EATEST_VERIFY(!eastl::in_range<unsigned short>(eastl::numeric_limits<int>::min()));

	EATEST_VERIFY(eastl::in_range<long>(50));
	EATEST_VERIFY(eastl::in_range<long>(eastl::numeric_limits<long>::min()));
	EATEST_VERIFY(eastl::in_range<long>(eastl::numeric_limits<long>::max()));
	EATEST_VERIFY(eastl::in_range<unsigned long>(50));
	EATEST_VERIFY(eastl::in_range<unsigned long>(eastl::numeric_limits<unsigned long>::min()));
	EATEST_VERIFY(eastl::in_range<unsigned long>(eastl::numeric_limits<unsigned long>::max()));
	EATEST_VERIFY(!eastl::in_range<unsigned long>(-1));
	EATEST_VERIFY(eastl::in_range<long>(eastl::numeric_limits<unsigned int>::max()) == (sizeof(unsigned int) < sizeof(long)));
	EATEST_VERIFY(!eastl::in_range<unsigned long>(eastl::numeric_limits<int>::min()));

	return nErrorCount;
}
#endif

///////////////////////////////////////////////////////////////////////////////
// TestUtility
//
int TestUtility()
{
	int nErrorCount = 0;

	nErrorCount += TestUtilityPair();
	nErrorCount += TestUtilityRelops();
	nErrorCount += TestUtilitySwap();
	nErrorCount += TestUtilityMove();
	nErrorCount += TestUtilityIntegerSequence();
	nErrorCount += TestUtilityExchange();
#if EASTL_INLINE_VARIABLE_ENABLED && EASTL_TUPLE_ENABLED && EASTL_OPTIONAL_ENABLED
	nErrorCount += TestUtilityTagObjects();
#endif
#if defined(EA_COMPILER_CPP17_ENABLED)
	nErrorCount += TestUtilityIntegralComp();
#endif
	return nErrorCount;
}
