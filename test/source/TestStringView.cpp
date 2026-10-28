/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "EASTLTest.h"
#include <EABase/eabase.h>
#include <EASTL/numeric_limits.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>

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


static EA_CPP14_CONSTEXPR bool TestConstexprStringViewFind()
{
	const char text[] = { 'a', 'b', '\0', 'c', 'd', 'a', 'b' };
	const eastl::string_view view(text, 7);
	const eastl::string_view empty;

	if(view.find("ab") != 0 || view.find("ab", 1) != 5 ||
	   view.find("ab", 6) != eastl::string_view::npos ||
	   view.find('b', 2) != 6 || view.find('\0') != 2 ||
	   view.find("\0c", 0, 2) != 2 ||
	   view.find("abcdefghi") != eastl::string_view::npos ||
	   view.find("ce") != eastl::string_view::npos)
		return false;

	if(view.find(empty, 3) != 3 || view.find("", 0) != 0 ||
	   view.find("", view.size()) != view.size() ||
	   view.find("", view.size() + 1) != eastl::string_view::npos ||
	   view.find("", eastl::string_view::npos) != eastl::string_view::npos ||
	   view.find('a', eastl::string_view::npos) != eastl::string_view::npos)
		return false;

	if(empty.find(empty) != 0 || empty.find("") != 0 ||
	   empty.find(empty, 1) != eastl::string_view::npos ||
	   empty.find(empty, eastl::string_view::npos) != eastl::string_view::npos ||
	   empty.find('a') != eastl::string_view::npos)
		return false;

	const char16_t utf16[] = { u'\u03b1', u'\0', u'\u03b2', u'\u03b1' };
	const eastl::u16string_view view16(utf16, 4);
	if(view16.find(u"\u03b2") != 2 || view16.find(u'\0') != 1 ||
	   view16.find(u"\0\u03b2", 0, 2) != 1 || view16.find(u"\u03b1", 1) != 3)
		return false;

	#if EA_CHAR8_UNIQUE
		const char8_t utf8[] = { u8'a', u8'\0', u8'b' };
		const eastl::u8string_view view8(utf8, 3);
		if(view8.find(u8"\0b", 0, 2) != 1 || view8.find(u8'b') != 2)
			return false;
	#endif

	return true;
}

static_assert(TestConstexprStringViewFind(), "string_view find overloads must support constant evaluation");


int TestStringView()
{
	using namespace eastl;
	int nErrorCount = 0;
	VERIFY(TestConstexprStringViewFind());

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


	return nErrorCount;
}

