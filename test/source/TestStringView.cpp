/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "EASTLTest.h"
#include <EABase/eabase.h>
#include <EASTL/numeric_limits.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>
#include <EASTL/vector.h>
#include <EASTL/array.h>

// The exception specifications of basic_string_view follow the standard (MSVC's STL strengthens the overloads that take
// a C string, operator[], front, back, remove_prefix and remove_suffix).
namespace NoexceptStringViewTest
{
	#define EASTL_TEST_NOEXCEPT_STRING_VIEW(C) \
		static_assert(noexcept(eastl::basic_string_view<C>()), "basic_string_view<" #C ">() is noexcept"); \
		static_assert(noexcept(eastl::basic_string_view<C>(static_cast<const C*>(nullptr))), "basic_string_view<" #C ">(const C*) is noexcept"); \
		static_assert(noexcept(eastl::basic_string_view<C>(static_cast<const C*>(nullptr), 1)), "basic_string_view<" #C ">(const C*, size_type) is noexcept"); \
		static_assert(noexcept(eastl::basic_string_view<C>(static_cast<const C*>(nullptr), static_cast<const C*>(nullptr))), "basic_string_view<" #C ">(It, End) is noexcept"); \
		static_assert(noexcept(eastl::basic_string_view<C>(eastl::declval<eastl::array<C, 3>&>())), "basic_string_view<" #C ">(range) is noexcept when data and size are"); \
		static_assert(noexcept(eastl::declval<const eastl::basic_string_view<C>&>().data()), "basic_string_view<" #C ">::data is noexcept"); \
		static_assert(noexcept(eastl::declval<const eastl::basic_string_view<C>&>().front()), "basic_string_view<" #C ">::front is noexcept"); \
		static_assert(noexcept(eastl::declval<const eastl::basic_string_view<C>&>().back()), "basic_string_view<" #C ">::back is noexcept"); \
		static_assert(noexcept(eastl::declval<const eastl::basic_string_view<C>&>()[0]), "basic_string_view<" #C ">::operator[] is noexcept"); \
		static_assert(noexcept(eastl::declval<eastl::basic_string_view<C>&>().remove_prefix(1)), "basic_string_view<" #C ">::remove_prefix is noexcept"); \
		static_assert(noexcept(eastl::declval<eastl::basic_string_view<C>&>().remove_suffix(1)), "basic_string_view<" #C ">::remove_suffix is noexcept"); \
		static_assert(noexcept(eastl::declval<eastl::basic_string_view<C>&>().swap(eastl::declval<eastl::basic_string_view<C>&>())), "basic_string_view<" #C ">::swap is noexcept"); \
		static_assert(noexcept(eastl::declval<const eastl::basic_string_view<C>&>().compare(static_cast<const C*>(nullptr))), "basic_string_view<" #C ">::compare(const C*) is noexcept"); \
		static_assert(noexcept(eastl::declval<const eastl::basic_string_view<C>&>().find(static_cast<const C*>(nullptr), 0, 1)), "basic_string_view<" #C ">::find(const C*, pos, count) is noexcept"); \
		static_assert(noexcept(eastl::declval<const eastl::basic_string_view<C>&>().find(static_cast<const C*>(nullptr), 0)), "basic_string_view<" #C ">::find(const C*, pos) is noexcept"); \
		static_assert(noexcept(eastl::declval<const eastl::basic_string_view<C>&>().rfind(static_cast<const C*>(nullptr), 0, 1)), "basic_string_view<" #C ">::rfind(const C*, pos, count) is noexcept"); \
		static_assert(noexcept(eastl::declval<const eastl::basic_string_view<C>&>().rfind(static_cast<const C*>(nullptr), 0)), "basic_string_view<" #C ">::rfind(const C*, pos) is noexcept"); \
		static_assert(noexcept(eastl::declval<const eastl::basic_string_view<C>&>().find_first_of(static_cast<const C*>(nullptr), 0, 1)), "basic_string_view<" #C ">::find_first_of(const C*, pos, count) is noexcept"); \
		static_assert(noexcept(eastl::declval<const eastl::basic_string_view<C>&>().find_first_of(static_cast<const C*>(nullptr), 0)), "basic_string_view<" #C ">::find_first_of(const C*, pos) is noexcept"); \
		static_assert(noexcept(eastl::declval<const eastl::basic_string_view<C>&>().find_last_of(static_cast<const C*>(nullptr), 0, 1)), "basic_string_view<" #C ">::find_last_of(const C*, pos, count) is noexcept"); \
		static_assert(noexcept(eastl::declval<const eastl::basic_string_view<C>&>().find_last_of(static_cast<const C*>(nullptr), 0)), "basic_string_view<" #C ">::find_last_of(const C*, pos) is noexcept"); \
		static_assert(noexcept(eastl::declval<const eastl::basic_string_view<C>&>().find_first_not_of(static_cast<const C*>(nullptr), 0, 1)), "basic_string_view<" #C ">::find_first_not_of(const C*, pos, count) is noexcept"); \
		static_assert(noexcept(eastl::declval<const eastl::basic_string_view<C>&>().find_first_not_of(static_cast<const C*>(nullptr), 0)), "basic_string_view<" #C ">::find_first_not_of(const C*, pos) is noexcept"); \
		static_assert(noexcept(eastl::declval<const eastl::basic_string_view<C>&>().find_last_not_of(static_cast<const C*>(nullptr), 0, 1)), "basic_string_view<" #C ">::find_last_not_of(const C*, pos, count) is noexcept"); \
		static_assert(noexcept(eastl::declval<const eastl::basic_string_view<C>&>().find_last_not_of(static_cast<const C*>(nullptr), 0)), "basic_string_view<" #C ">::find_last_not_of(const C*, pos) is noexcept"); \
		static_assert(noexcept(eastl::declval<const eastl::basic_string_view<C>&>().starts_with(static_cast<const C*>(nullptr))), "basic_string_view<" #C ">::starts_with(const C*) is noexcept"); \
		static_assert(noexcept(eastl::declval<const eastl::basic_string_view<C>&>().ends_with(static_cast<const C*>(nullptr))), "basic_string_view<" #C ">::ends_with(const C*) is noexcept"); \
		static_assert(noexcept(eastl::declval<const eastl::basic_string_view<C>&>() == static_cast<const C*>(nullptr)), "basic_string_view<" #C "> == const C* is noexcept"); \
		static_assert(noexcept(eastl::hash<eastl::basic_string_view<C> >()(eastl::declval<const eastl::basic_string_view<C>&>())), "hash<basic_string_view<" #C ">>::operator() is noexcept");
	EASTL_TEST_NOEXCEPT_STRING_VIEW(char)
	EASTL_TEST_NOEXCEPT_STRING_VIEW(wchar_t)
	EASTL_TEST_NOEXCEPT_STRING_VIEW(char16_t)
	EASTL_TEST_NOEXCEPT_STRING_VIEW(char32_t)
	#if EA_CHAR8_UNIQUE
		EASTL_TEST_NOEXCEPT_STRING_VIEW(char8_t)
	#endif
	#undef EASTL_TEST_NOEXCEPT_STRING_VIEW
}

// Verify char8_t support is present if the test build requested it.
#if defined(EASTL_EXPECT_CHAR8T_SUPPORT) && !EA_CHAR8_UNIQUE
static_assert(false, "Building with char8_t tests enabled, but EA_CHAR8_UNIQUE evaluates to false.");
#endif

// this mess is required inorder to inject string literal string conversion macros into the unit tests
#define TEST_STRING_NAME TestBasicStringView
#define LITERAL(x) x
#include "TestStringView.inl"

#define TEST_STRING_NAME TestBasicStringViewW
#define LITERAL(x) EA_WCHAR(x)
#include "TestStringView.inl"

#define TEST_STRING_NAME TestBasicStringView8
#define LITERAL(x) EA_CHAR8(x)
#include "TestStringView.inl"

#define TEST_STRING_NAME TestBasicStringView16
#define LITERAL(x) EA_CHAR16(x)
#include "TestStringView.inl"

#define TEST_STRING_NAME TestBasicStringView32
#define LITERAL(x) EA_CHAR32(x)
#include "TestStringView.inl"


// string_view's find overloads support constant evaluation, including embedded NULs.
static constexpr eastl::string_view kFindView("ab\0cdab", 7);
static_assert(kFindView.find("ab", 1) == 5, "find a substring");
static_assert(kFindView.find("\0c", 0, 2) == 2, "find a substring with an embedded NUL");
static_assert(kFindView.find("abcdefghi") == eastl::string_view::npos, "find a substring longer than the view");
static_assert(kFindView.find("", kFindView.size()) == kFindView.size(), "find an empty string at the end");
static_assert(kFindView.find("", kFindView.size() + 1) == eastl::string_view::npos, "find an empty string past the end");
static_assert(eastl::u16string_view(u"\u03b1\u03b2\u03b1").find(u"\u03b1", 1) == 2, "find in a UTF-16 view");


int TestStringView()
{
	using namespace eastl;
	int nErrorCount = 0;

	nErrorCount += TestBasicStringView<eastl::basic_string_view<char>>();
	nErrorCount += TestBasicStringView<eastl::string_view>();

	nErrorCount += TestBasicStringViewW<eastl::basic_string_view<wchar_t>>();
	nErrorCount += TestBasicStringViewW<eastl::wstring_view>();

#if EA_CHAR8_UNIQUE
	nErrorCount += TestBasicStringView8<eastl::basic_string_view<char8_t>>();
	nErrorCount += TestBasicStringView8<eastl::u8string_view>();
#endif

	nErrorCount += TestBasicStringView16<eastl::basic_string_view<char16_t>>();
	nErrorCount += TestBasicStringView16<eastl::u16string_view>();

#if EA_CHAR32_NATIVE
	nErrorCount += TestBasicStringView32<eastl::basic_string_view<char32_t>>();
	nErrorCount += TestBasicStringView32<eastl::u32string_view>();
#endif


	// constexpr string_view operator "" sv(const char* str, size_t len) noexcept;
	// constexpr u8string_view operator "" sv(const char8_t* str, size_t len) noexcept;
	// constexpr u16string_view operator "" sv(const char16_t* str, size_t len) noexcept;
	// constexpr u32string_view operator "" sv(const char32_t* str, size_t len) noexcept;
	// constexpr wstring_view   operator "" sv(const wchar_t* str, size_t len) noexcept;
	#if EASTL_USER_LITERALS_ENABLED 
	{
		VERIFY("cplusplus"_sv.compare("cplusplus") == 0);
		VERIFY(L"cplusplus"_sv.compare(L"cplusplus") == 0);
		VERIFY(u"cplusplus"_sv.compare(u"cplusplus") == 0);
		VERIFY(U"cplusplus"_sv.compare(U"cplusplus") == 0);
		VERIFY(u8"cplusplus"_sv.compare(u8"cplusplus") == 0);

		static_assert(eastl::is_same_v<decltype("abcdef"_sv), eastl::string_view>, "string_view literal type mismatch");
		static_assert(eastl::is_same_v<decltype(u8"abcdef"_sv), eastl::u8string_view>, "string_view literal type mismatch");
		static_assert(eastl::is_same_v<decltype(u"abcdef"_sv), eastl::u16string_view>, "string_view literal type mismatch");
		static_assert(eastl::is_same_v<decltype(U"abcdef"_sv), eastl::u32string_view>, "string_view literal type mismatch");
		static_assert(eastl::is_same_v<decltype(L"abcdef"_sv), eastl::wstring_view>, "string_view literal type mismatch");


		VERIFY("cplusplus"sv.compare("cplusplus") == 0);
		VERIFY(L"cplusplus"sv.compare(L"cplusplus") == 0);
		VERIFY(u"cplusplus"sv.compare(u"cplusplus") == 0);
		VERIFY(U"cplusplus"sv.compare(U"cplusplus") == 0);
		VERIFY(u8"cplusplus"sv.compare(u8"cplusplus") == 0);

		static_assert(eastl::is_same_v<decltype("abcdef"sv), eastl::string_view>, "string_view literal type mismatch");
		static_assert(eastl::is_same_v<decltype(u8"abcdef"sv), eastl::u8string_view>, "string_view literal type mismatch");
		static_assert(eastl::is_same_v<decltype(u"abcdef"sv), eastl::u16string_view>, "string_view literal type mismatch");
		static_assert(eastl::is_same_v<decltype(U"abcdef"sv), eastl::u32string_view>, "string_view literal type mismatch");
		static_assert(eastl::is_same_v<decltype(L"abcdef"sv), eastl::wstring_view>, "string_view literal type mismatch");
	}
	#endif


	// strlen(char_t) compatibility
	{
		auto* pStr = "Hello, World";
		string_view sw(pStr, strlen(pStr));
		VERIFY(sw.size() == strlen(pStr));
	}

	// strlen(wchar_t) compatibility
	{
		auto* pStr = L"Hello, World";
		wstring_view sw(pStr, wcslen(pStr));
		VERIFY(sw.size() == wcslen(pStr));
	}

	// The C++20/23 rules, like std: no construction or assignment from nullptr, the iterator-pair constructor and the
	// explicit range constructor.
	{
		static_assert(!is_constructible<string_view, std::nullptr_t>::value && !is_constructible<u16string_view, std::nullptr_t>::value, "string_view(nullptr) is deleted");
		static_assert(!is_constructible<string, std::nullptr_t>::value && !is_constructible<wstring, std::nullptr_t>::value && !is_assignable<string&, std::nullptr_t>::value, "string(nullptr) is deleted");
		static_assert(is_constructible<string_view, const char*, const char*>::value && is_constructible<string_view, char*, char*>::value && is_constructible<string_view, const char*, size_t>::value, "(It, End) and (s, count)");

		const char* p = "abcdef";
		string_view a(p, p + 3);
		VERIFY(a == "abc");

		static_assert(is_constructible<string_view, vector<char>&>::value && !is_convertible<vector<char>&, string_view>::value, "the range constructor is explicit");
		static_assert(!is_constructible<string_view, vector<wchar_t>&>::value, "not for a range of another character type");
		vector<char> v = { 'x', 'y' };
		string_view b(v);
		VERIFY((b.size() == 2) && (b[1] == 'y') && (b.data() == v.data()));

		// Not for a type with its own conversion to string_view, or a character array, which converts to const char*.
		string s("str");
		string_view c = s;
		char arr[] = "hi";
		string_view d(arr);
		VERIFY((c.data() == s.data()) && (d.size() == 2));
	}

	return nErrorCount;
}

