/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "EASTLTest.h"
#include <EASTL/array.h>
#include <EASTL/span.h>
#include <EASTL/vector.h>

// The exception specifications of span's constructors from a pointer, its subviews and its element access follow the
// standard (MSVC's STL strengthens them).
namespace NoexceptSpanTest
{
	#define EASTL_TEST_NOEXCEPT_SPAN(T, X) \
		static_assert(noexcept(eastl::declval<const eastl::span<T, X>&>().front()), "span<" #T ", " #X ">::front is noexcept"); \
		static_assert(noexcept(eastl::declval<const eastl::span<T, X>&>().back()), "span<" #T ", " #X ">::back is noexcept"); \
		static_assert(noexcept(eastl::declval<const eastl::span<T, X>&>()[0]), "span<" #T ", " #X ">::operator[] is noexcept"); \
		static_assert(noexcept(eastl::declval<const eastl::span<T, X>&>()(0)), "span<" #T ", " #X ">::operator() is noexcept"); \
		static_assert(noexcept(eastl::declval<const eastl::span<T, X>&>().template first<1>()), "span<" #T ", " #X ">::first<Count> is noexcept"); \
		static_assert(noexcept(eastl::declval<const eastl::span<T, X>&>().first(1)), "span<" #T ", " #X ">::first(Count) is noexcept"); \
		static_assert(noexcept(eastl::declval<const eastl::span<T, X>&>().template last<1>()), "span<" #T ", " #X ">::last<Count> is noexcept"); \
		static_assert(noexcept(eastl::declval<const eastl::span<T, X>&>().last(1)), "span<" #T ", " #X ">::last(Count) is noexcept"); \
		static_assert(noexcept(eastl::declval<const eastl::span<T, X>&>().template subspan<0>()), "span<" #T ", " #X ">::subspan<Offset> is noexcept"); \
		static_assert(noexcept(eastl::declval<const eastl::span<T, X>&>().subspan(0, 1)), "span<" #T ", " #X ">::subspan(Offset, Count) is noexcept"); \
		static_assert(noexcept(eastl::span<T, X>(static_cast<T*>(nullptr), 3)), "span<" #T ", " #X ">(pointer, count) is noexcept"); \
		static_assert(noexcept(eastl::span<T, X>(static_cast<T*>(nullptr), static_cast<T*>(nullptr))), "span<" #T ", " #X ">(pointer, pointer) is noexcept");
	EASTL_TEST_NOEXCEPT_SPAN(int, eastl::dynamic_extent)
	EASTL_TEST_NOEXCEPT_SPAN(int, 3)
	EASTL_TEST_NOEXCEPT_SPAN(const char, eastl::dynamic_extent)
	#undef EASTL_TEST_NOEXCEPT_SPAN

	// The constructors from a container are noexcept when the container's data() and size() are.
	template <bool Nothrow>
	struct Container
	{
		int* data() EA_NOEXCEPT_IF(Nothrow);
		const int* data() const EA_NOEXCEPT_IF(Nothrow);
		size_t size() const EA_NOEXCEPT_IF(Nothrow);
	};
	static_assert(noexcept(eastl::span<int>(eastl::declval<Container<true>&>())), "span(Container&) is noexcept when data and size are");
	static_assert(noexcept(eastl::span<const int>(eastl::declval<const Container<true>&>())), "span(const Container&) is noexcept when data and size are");
	static_assert(noexcept(eastl::span<int, 3>(eastl::declval<Container<true>&>())), "the explicit span(Container&) is noexcept when data and size are");
	static_assert(noexcept(eastl::span<const int, 3>(eastl::declval<const Container<true>&>())), "the explicit span(const Container&) is noexcept when data and size are");
	static_assert(!noexcept(eastl::span<int>(eastl::declval<Container<false>&>())), "span(Container&) may throw when data or size may");
	static_assert(!noexcept(eastl::span<const int>(eastl::declval<const Container<false>&>())), "span(const Container&) may throw when data or size may");
	static_assert(!noexcept(eastl::span<int, 3>(eastl::declval<Container<false>&>())), "the explicit span(Container&) may throw when data or size may");
	static_assert(noexcept(eastl::Internal::SpanStorage<int, eastl::dynamic_extent>(static_cast<int*>(nullptr), 1)), "SpanStorage(pointer, size) is noexcept");
	static_assert(noexcept(eastl::Internal::SpanStorage<int, 3>(static_cast<int*>(nullptr), 3)), "SpanStorage<T, Extent>(pointer, size) is noexcept");
}

void TestSpanCtor(int& nErrorCount)
{
	using namespace eastl;

	{
		span<int> s;
		VERIFY(s.empty());
		VERIFY(s.size() == 0);
		VERIFY(s.data() == nullptr);
	}
	{
		span<float> s;
		VERIFY(s.empty());
		VERIFY(s.size() == 0);
		VERIFY(s.data() == nullptr);
	}
	{
		span<TestObject> s;
		VERIFY(s.empty());
		VERIFY(s.size() == 0);
		VERIFY(s.data() == nullptr);
	}

	{
		int arr[5] = {0, 1, 2, 3, 4};
		span<int> s(eastl::begin(arr), 5);
		VERIFY(s.data() == eastl::begin(arr));
		VERIFY(s.size() == 5);
		VERIFY(!s.empty());
	}

	{
		int arr[5] = {0, 1, 2, 3, 4};
		span<int> s(eastl::begin(arr), eastl::end(arr));
		VERIFY(s.data() == eastl::begin(arr));
		VERIFY(s.size() == 5);
		VERIFY(!s.empty());
	}

	{
		int arr[5] = {0, 1, 2, 3, 4};
		span<int> s(arr);
		VERIFY(s.data() == eastl::begin(arr));
		VERIFY(s.size() == 5);
		VERIFY(s.data()[2] == arr[2]);
		VERIFY(!s.empty());
	}

	{
		eastl::array<int, 5> arr = {{0, 1, 2, 3, 4}};
		span<int> s(arr);
		VERIFY(s.data() == eastl::begin(arr));
		VERIFY(s.size() == 5);
		VERIFY(s.data()[2] == arr.data()[2]);
		VERIFY(!s.empty());
	}

	{
		const eastl::array<int, 5> arr = {{0, 1, 2, 3, 4}};
		span<const int> s(arr);
		VERIFY(s.data() == eastl::begin(arr));
		VERIFY(s.size() == 5);
		VERIFY(s.data()[2] == arr.data()[2]);
		VERIFY(!s.empty());
	}

	{
		const eastl::array<int, 5> arr = {{0, 1, 2, 3, 4}};
		const span<const int> s(arr);
		VERIFY(s.data() == eastl::begin(arr));
		VERIFY(s.size() == 5);
		VERIFY(s.data()[2] == arr.data()[2]);
	}

	{
		class foo {};

		foo* pFoo = nullptr;

		auto f = [](eastl::span<const foo*>) {};

		eastl::array<const foo*, 1> foos = {{pFoo}};

		f(foos);
	}
}

void TestSpanSizeBytes(int& nErrorCount)
{
	using namespace eastl;

	{
		int arr[5] = {0, 1, 2, 3, 4};
		span<int> s(arr);
		VERIFY(s.size_bytes() == sizeof(arr));
		VERIFY(s.size_bytes() == (5 * sizeof(int)));
	}

	{
		float arr[8] = {0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f};
		span<float> s(arr);
		VERIFY(s.size_bytes() == sizeof(arr));
		VERIFY(s.size_bytes() == (8 * sizeof(float)));
	}

	{
		int64_t arr[5] = {0, 1, 2, 3, 4};
		span<int64_t> s(arr);
		VERIFY(s.size_bytes() == sizeof(arr));
		VERIFY(s.size_bytes() == (5 * sizeof(int64_t)));
	}
}

void TestSpanElementAccess(int& nErrorCount)
{
	using namespace eastl;

	{
		int arr[5] = {0, 1, 2, 3, 4};
		span<int> s(arr);

		VERIFY(s.front() == 0);
		VERIFY(s.back() == 4);

		VERIFY(s[0] == 0);
		VERIFY(s[1] == 1);
		VERIFY(s[2] == 2);
		VERIFY(s[3] == 3);
		VERIFY(s[4] == 4);

		VERIFY(s(0) == 0);
		VERIFY(s(1) == 1);
		VERIFY(s(2) == 2);
		VERIFY(s(3) == 3);
		VERIFY(s(4) == 4);
	}
}

void TestSpanIterators(int& nErrorCount)
{
	using namespace eastl;

	int arr[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	span<int> s(arr);

	// ranged-for test
	{
		int* pBegin = arr;
		for(auto& e : arr)
		{
			VERIFY(e == *pBegin++);
		}
	}

	{
		auto testIteratorBegin = [&](auto p)
		{
			VERIFY(*p++ == 0);
			VERIFY(*p++ == 1);
			VERIFY(*p++ == 2);
			VERIFY(*p++ == 3);
			VERIFY(*p++ == 4);
			VERIFY(*p++ == 5);
			VERIFY(*p++ == 6);
			VERIFY(*p++ == 7);
			VERIFY(*p++ == 8);
			VERIFY(*p++ == 9);
		};

		auto testIteratorEnd = [&](auto p)
		{
			p--; // move pointer to a valid element

			VERIFY(*p-- == 9);
			VERIFY(*p-- == 8);
			VERIFY(*p-- == 7);
			VERIFY(*p-- == 6);
			VERIFY(*p-- == 5);
			VERIFY(*p-- == 4);
			VERIFY(*p-- == 3);
			VERIFY(*p-- == 2);
			VERIFY(*p-- == 1);
			VERIFY(*p-- == 0);
		};

		testIteratorBegin(s.begin());
		testIteratorBegin(s.cbegin());
		testIteratorEnd(s.end());
		testIteratorEnd(s.cend());
	}

	{
		auto testReverseIteratorBegin = [&](auto p)
		{
			VERIFY(*p++ == 9);
			VERIFY(*p++ == 8);
			VERIFY(*p++ == 7);
			VERIFY(*p++ == 6);
			VERIFY(*p++ == 5);
			VERIFY(*p++ == 4);
			VERIFY(*p++ == 3);
			VERIFY(*p++ == 2);
			VERIFY(*p++ == 1);
			VERIFY(*p++ == 0);
		};

		auto testReverseIteratorEnd = [&](auto p)
		{
			p--; // move pointer to a valid element

			VERIFY(*p-- == 0);
			VERIFY(*p-- == 1);
			VERIFY(*p-- == 2);
			VERIFY(*p-- == 3);
			VERIFY(*p-- == 4);
			VERIFY(*p-- == 5);
			VERIFY(*p-- == 6);
			VERIFY(*p-- == 7);
			VERIFY(*p-- == 8);
			VERIFY(*p-- == 9);
		};

		testReverseIteratorBegin(s.rbegin());
		testReverseIteratorBegin(s.crbegin());
		testReverseIteratorEnd(s.rend());
		testReverseIteratorEnd(s.crend());
	}
}

void TestSpanCopyAssignment(int& nErrorCount)
{
	using namespace eastl;

	{
		int arr[5] = {0, 1, 2, 3, 4};
		span<int> s(arr);
		span<int> sc = s;

		VERIFY(s[0] == sc[0]);
		VERIFY(s[1] == sc[1]);
		VERIFY(s[2] == sc[2]);
		VERIFY(s[3] == sc[3]);
		VERIFY(s[4] == sc[4]);

		VERIFY(s(0) == sc(0));
		VERIFY(s(1) == sc(1));
		VERIFY(s(2) == sc(2));
		VERIFY(s(3) == sc(3));
		VERIFY(s(4) == sc(4));
	}
}

void TestSpanContainerConversion(int& nErrorCount)
{
	using namespace eastl;

	{
		vector<int> v = {0, 1, 2, 3, 4, 5};
		span<const int> s(v);

		VERIFY(s.size() == static_cast<span<int>::index_type>(eastl::size(v)));
		VERIFY(s.data() == eastl::data(v));

		VERIFY(s[0] == v[0]);
		VERIFY(s[1] == v[1]);
		VERIFY(s[2] == v[2]);
		VERIFY(s[3] == v[3]);
		VERIFY(s[4] == v[4]);
		VERIFY(s[5] == v[5]);
	}

	{
		const vector<int> v = {0, 1, 2, 3, 4, 5};
		span<const int> s(v);

		VERIFY(s.size() == static_cast<span<int>::index_type>(eastl::size(v)));
		VERIFY(s.data() == eastl::data(v));

		VERIFY(s[0] == v[0]);
		VERIFY(s[1] == v[1]);
		VERIFY(s[2] == v[2]);
		VERIFY(s[3] == v[3]);
		VERIFY(s[4] == v[4]);
		VERIFY(s[5] == v[5]);
	}

	{
		vector<int> v = {0, 1, 2, 3, 4, 5};
		span<const int, 6> s1(v);
		span<const int> s2(s1);
		
		VERIFY(s2.size() == (span<const int>::index_type)v.size());
		VERIFY(s2[0] == v[0]);
		VERIFY(s2[1] == v[1]);

		VERIFY(s1.data() ==  v.data());
		VERIFY(s1.data() == s2.data());
	}

	{ // user reported regression for calling non-const span overload with a vector.
		auto f1 = [](span<int> s) { return s.size(); };
		auto f2 = [](span<const int> s) { return s.size(); };

		{
			vector<int> v = {0, 1, 2, 3, 4, 5};

			VERIFY(f1(v) == v.size());
			VERIFY(f2(v) == v.size());
		}

		{
			int a[] = {0, 1, 2, 3, 4, 5};

			VERIFY(f1(a) == EAArrayCount(a));
			VERIFY(f2(a) == EAArrayCount(a));
		}
	}
}

void TestSpanComparison(int& nErrorCount)
{
	using namespace eastl;

	int arr1[5] = {0, 1, 2, 3, 4};
	int arr2[8] = {0, 1, 2, 3, 4, 5, 6, 7};
	{
		span<int> s1 = arr1;
		span<int> s2 = arr2;
		span<int> s3 = arr2;
		VERIFY(s2 == s3);
		VERIFY(s1 != s2);
		VERIFY(s1 < s2);
		VERIFY(s1 <= s2);
		VERIFY(s2 > s1);
		VERIFY(s2 >= s1);
	}
}

void TestSpanSubViews(int& nErrorCount)
{
	using namespace eastl;

	int arr1[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

	{
		span<int> s =  arr1;
		auto first_span = s.first<4>();
		VERIFY(first_span.size() == 4);
		VERIFY(first_span[0] == 0);
		VERIFY(first_span[1] == 1);
		VERIFY(first_span[2] == 2);
		VERIFY(first_span[3] == 3);
	}

	{
		span<int> s =  arr1;
		auto first_span = s.first(4);
		VERIFY(first_span.size() == 4);
		VERIFY(first_span[0] == 0);
		VERIFY(first_span[1] == 1);
		VERIFY(first_span[2] == 2);
		VERIFY(first_span[3] == 3);
	}

	{
		span<int> s =  arr1;
		auto first_span = s.last<4>();
		VERIFY(first_span.size() == 4);
		VERIFY(first_span[0] == 6);
		VERIFY(first_span[1] == 7);
		VERIFY(first_span[2] == 8);
		VERIFY(first_span[3] == 9);
	}

	{
		span<int> s =  arr1;
		auto first_span = s.last(4);
		VERIFY(first_span.size() == 4);
		VERIFY(first_span[0] == 6);
		VERIFY(first_span[1] == 7);
		VERIFY(first_span[2] == 8);
		VERIFY(first_span[3] == 9);
	}

	{ // empty range
		span<int, 0> s{};

		auto fixed_span = s.subspan<0, 0>();
		VERIFY(fixed_span.empty());
		fixed_span = s.first<0>();
		VERIFY(fixed_span.empty());
		fixed_span = s.last<0>();
		VERIFY(fixed_span.empty());

		span<int> dynamic_span;
		VERIFY(dynamic_span.empty());
		dynamic_span = s.first(0);
		VERIFY(dynamic_span.empty());
		dynamic_span = s.last(0);
		VERIFY(dynamic_span.empty());
	}

	{ // subspan: full range
		span<int, 10> s =  arr1;

		auto fixed_span = s.subspan<0, 10>();
		VERIFY(fixed_span.size() == 10);
		VERIFY(fixed_span[0] == 0);
		VERIFY(fixed_span[1] == 1);
		VERIFY(fixed_span[8] == 8);
		VERIFY(fixed_span[9] == 9);

		auto dynamic_span = s.subspan(0, s.size());
		VERIFY(dynamic_span.size() == 10);
		VERIFY(dynamic_span[0] == 0);
		VERIFY(dynamic_span[1] == 1);
		VERIFY(dynamic_span[8] == 8);
		VERIFY(dynamic_span[9] == 9);
	}

	{ // subspan: subrange
		span<int, 10> s =  arr1;

		auto fixed_span = s.subspan<3, 4>();
		VERIFY(fixed_span.size() == 4);
		VERIFY(fixed_span[0] == 3);
		VERIFY(fixed_span[1] == 4);
		VERIFY(fixed_span[2] == 5);
		VERIFY(fixed_span[3] == 6);

		auto dynamic_span = s.subspan(3, 4);
		VERIFY(dynamic_span.size() == 4);
		VERIFY(dynamic_span[0] == 3);
		VERIFY(dynamic_span[1] == 4);
		VERIFY(dynamic_span[2] == 5);
		VERIFY(dynamic_span[3] == 6);
	}

	{ // subspan: default count
		span<int, 10> s =  arr1;

		auto fixed_span = s.subspan<3>();
		VERIFY(fixed_span.size() == 7);
		VERIFY(fixed_span[0] == 3);
		VERIFY(fixed_span[1] == 4);
		VERIFY(fixed_span[5] == 8);
		VERIFY(fixed_span[6] == 9);

		auto dynamic_span = s.subspan(3);
		VERIFY(dynamic_span.size() == 7);
		VERIFY(dynamic_span[0] == 3);
		VERIFY(dynamic_span[1] == 4);
		VERIFY(dynamic_span[5] == 8);
		VERIFY(dynamic_span[6] == 9);
	}
}

template <typename T>
using as_writable_bytes_t = decltype(eastl::as_writable_bytes(eastl::declval<T>()));

// The C++20 rules: explicit construction of a span of static extent, constructors from arrays of any element type,
// and as_bytes / as_writable_bytes, like std.
void TestSpanCpp20Rules(int& nErrorCount)
{
	using namespace eastl;

	static_assert(!is_convertible<span<int>, span<int, 3>>::value && is_constructible<span<int, 3>, span<int>>::value, "span -> span of static extent is explicit");
	static_assert(!is_convertible<vector<int>&, span<int, 3>>::value && is_constructible<span<int, 3>, vector<int>&>::value, "container -> span of static extent is explicit");
	static_assert(is_convertible<array<int, 3>&, span<int, 3>>::value && !is_constructible<span<int, 3>, array<int, 4>&>::value, "array -> span");
	static_assert(is_convertible<const array<int, 3>&, span<const int, 3>>::value && !is_constructible<span<int>, const array<int, 3>&>::value, "const array -> span");
	static_assert(is_convertible<array<int, 3>&, span<const int>>::value && is_convertible<span<int>, span<const int>>::value, "span of const elements");
	static_assert(!is_detected<as_writable_bytes_t, span<const int>>::value, "as_writable_bytes of const elements");

	int a[4] = { 1, 2, 3, 4 };
	span<int, 3> s1(a, 3);
	span<int, 3> s2(a, a + 3);
	span<int, 3> s3(span<int>(a, 3));
	VERIFY((s1.size() == 3) && (s2.data() == a) && (s3[2] == 3));

	auto first = s1.first<2>();
	auto last = s1.last<2>();
	auto sub = s1.subspan<1, 1>();
	static_assert(decltype(first)::extent == 2 && decltype(last)::extent == 2 && decltype(sub)::extent == 1, "static subviews");
	VERIFY((first[1] == 2) && (last[0] == 2) && (sub[0] == 2));

	auto bytes = as_bytes(s1);
	static_assert(decltype(bytes)::extent == 3 * sizeof(int) && is_same<decltype(bytes)::element_type, const std::byte>::value, "as_bytes");
	VERIFY((bytes.size() == 3 * sizeof(int)) && (static_cast<const void*>(bytes.data()) == static_cast<const void*>(a)));

	auto writable = as_writable_bytes(span<int>(a, 4));
	static_assert(decltype(writable)::extent == dynamic_extent, "as_writable_bytes of a dynamic span");
	for (size_t i = 0; i < sizeof(int); i++)
		writable[i] = std::byte{0};
	VERIFY((a[0] == 0) && (a[1] == 2));
}

int TestSpan()
{
	int nErrorCount = 0;

	TestSpanCpp20Rules(nErrorCount);

	TestSpanCtor(nErrorCount);
	TestSpanSizeBytes(nErrorCount);
	TestSpanElementAccess(nErrorCount);
	TestSpanIterators(nErrorCount);
	TestSpanCopyAssignment(nErrorCount);
	TestSpanContainerConversion(nErrorCount);
	TestSpanComparison(nErrorCount);
	TestSpanSubViews(nErrorCount);

	return nErrorCount;
}
