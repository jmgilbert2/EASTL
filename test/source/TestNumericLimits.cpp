/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#include "EASTLTest.h"
#include <EASTL/numeric_limits.h>
#include <EASTL/numeric.h>
#include <float.h>


///////////////////////////////////////////////////////////////////////////////
// TestNumericLimits
//
int TestNumericLimits()
{
	int nErrorCount = 0;

	// To consider: Some day when we get more time, make a big table-driven set of 
	// expected results to all member variables and function calls.

	// Test bool in all const-volatile variants.
	EATEST_VERIFY(eastl::numeric_limits<bool>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<bool>::max() != 0);

	EATEST_VERIFY(eastl::numeric_limits<const bool>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<const bool>::max() != 0);

	EATEST_VERIFY(eastl::numeric_limits<volatile bool>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<volatile bool>::max() != 0);

	EATEST_VERIFY(eastl::numeric_limits<const volatile bool>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<const volatile bool>::max() != 0);

	// Do basic tests of the remaining types.
	EATEST_VERIFY(eastl::numeric_limits<char>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<char>::max() != 0);

	EATEST_VERIFY(eastl::numeric_limits<unsigned char>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<unsigned char>::max() != 0);

	EATEST_VERIFY(eastl::numeric_limits<signed char>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<signed char>::max() != 0);

	EATEST_VERIFY(eastl::numeric_limits<wchar_t>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<wchar_t>::max() != 0);

	#if defined(EA_CHAR8_UNIQUE) && EA_CHAR8_UNIQUE
	EATEST_VERIFY(eastl::numeric_limits<char8_t>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<char8_t>::max() != 0);
	#endif

	EATEST_VERIFY(eastl::numeric_limits<char16_t>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<char16_t>::max() != 0);

	EATEST_VERIFY(eastl::numeric_limits<char32_t>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<char32_t>::max() != 0);

	EATEST_VERIFY(eastl::numeric_limits<unsigned short>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<unsigned short>::max() != 0);

	EATEST_VERIFY(eastl::numeric_limits<signed short>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<signed short>::max() != 0);

	EATEST_VERIFY(eastl::numeric_limits<unsigned int>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<unsigned int>::max() != 0);

	EATEST_VERIFY(eastl::numeric_limits<signed int>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<signed int>::max() != 0);

	EATEST_VERIFY(eastl::numeric_limits<unsigned long>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<unsigned long>::max() != 0);

	EATEST_VERIFY(eastl::numeric_limits<signed long>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<signed long>::max() != 0);

	EATEST_VERIFY(eastl::numeric_limits<unsigned long long>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<unsigned long long>::max() != 0);

	EATEST_VERIFY(eastl::numeric_limits<signed long long>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<signed long long>::max() != 0);

	EATEST_VERIFY(eastl::numeric_limits<float>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<float>::max() != 0);

	EATEST_VERIFY(eastl::numeric_limits<double>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<double>::max() != 0);

	EATEST_VERIFY(eastl::numeric_limits<long double>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<long double>::max() != 0);

	// We don't yet have a generic global way to identify what the name of the supported 128 bit type is. 
	// We just happen to know that for gcc/clang it is __int128.
	#if (EA_COMPILER_INTMAX_SIZE >= 16) && (defined(EA_COMPILER_GNUC) || defined(EA_COMPILER_CLANG)) // If __int128_t/__uint128_t is supported...
		EATEST_VERIFY(eastl::numeric_limits<__uint128_t>::is_bounded);
		EATEST_VERIFY(eastl::numeric_limits<__uint128_t>::max() != 0);

		EATEST_VERIFY(eastl::numeric_limits<__int128_t>::is_bounded);
		EATEST_VERIFY(eastl::numeric_limits<__int128_t>::max() != 0);
	#endif

	// Test sized types.
	EATEST_VERIFY(eastl::numeric_limits<uint8_t>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<uint8_t>::max() != 0);

	EATEST_VERIFY(eastl::numeric_limits<int8_t>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<int8_t>::max() != 0);

	EATEST_VERIFY(eastl::numeric_limits<uint16_t>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<uint16_t>::max() != 0);

	EATEST_VERIFY(eastl::numeric_limits<int16_t>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<int16_t>::max() != 0);

	EATEST_VERIFY(eastl::numeric_limits<uint32_t>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<uint32_t>::max() != 0);

	EATEST_VERIFY(eastl::numeric_limits<int32_t>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<int32_t>::max() != 0);

	EATEST_VERIFY(eastl::numeric_limits<uint64_t>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<uint64_t>::max() != 0);

	EATEST_VERIFY(eastl::numeric_limits<int64_t>::is_bounded);
	EATEST_VERIFY(eastl::numeric_limits<int64_t>::max() != 0);

	EATEST_VERIFY(eastl::isnan(eastl::numeric_limits<float>::quiet_NaN()));
	EATEST_VERIFY(eastl::isnan(eastl::numeric_limits<float>::signaling_NaN()));

	// max_digits10 is the number of decimal digits needed to round-trip a floating point value, and 0 for integers.
	#if defined(FLT_DECIMAL_DIG) && defined(DBL_DECIMAL_DIG) && defined(LDBL_DECIMAL_DIG) // C11 / C++17 macros.
		EATEST_VERIFY(eastl::numeric_limits<float>::max_digits10 == FLT_DECIMAL_DIG);
		EATEST_VERIFY(eastl::numeric_limits<double>::max_digits10 == DBL_DECIMAL_DIG);
		EATEST_VERIFY(eastl::numeric_limits<long double>::max_digits10 == LDBL_DECIMAL_DIG);
	#endif
	EATEST_VERIFY(eastl::numeric_limits<int>::max_digits10 == 0);

	// is_modulo is true for unsigned integer types and false for signed ones, whose overflow is undefined behavior.
	EATEST_VERIFY(!eastl::numeric_limits<bool>::is_modulo);
	EATEST_VERIFY(eastl::numeric_limits<char>::is_modulo == !eastl::numeric_limits<char>::is_signed);
	EATEST_VERIFY(!eastl::numeric_limits<signed char>::is_modulo);
	EATEST_VERIFY(eastl::numeric_limits<unsigned char>::is_modulo);
	EATEST_VERIFY(eastl::numeric_limits<wchar_t>::is_modulo == !eastl::numeric_limits<wchar_t>::is_signed);
	#if defined(EA_CHAR8_UNIQUE) && EA_CHAR8_UNIQUE
	EATEST_VERIFY(eastl::numeric_limits<char8_t>::is_modulo);
	#endif
	EATEST_VERIFY(eastl::numeric_limits<char16_t>::is_modulo);
	EATEST_VERIFY(eastl::numeric_limits<char32_t>::is_modulo);
	EATEST_VERIFY(!eastl::numeric_limits<signed short>::is_modulo);
	EATEST_VERIFY(eastl::numeric_limits<unsigned short>::is_modulo);
	EATEST_VERIFY(!eastl::numeric_limits<signed int>::is_modulo);
	EATEST_VERIFY(eastl::numeric_limits<unsigned int>::is_modulo);
	EATEST_VERIFY(!eastl::numeric_limits<signed long>::is_modulo);
	EATEST_VERIFY(eastl::numeric_limits<unsigned long>::is_modulo);
	EATEST_VERIFY(!eastl::numeric_limits<signed long long>::is_modulo);
	EATEST_VERIFY(eastl::numeric_limits<unsigned long long>::is_modulo);
	#if (EA_COMPILER_INTMAX_SIZE >= 16) && (defined(EA_COMPILER_GNUC) || defined(EA_COMPILER_CLANG)) // If __int128_t/__uint128_t is supported...
		EATEST_VERIFY(!eastl::numeric_limits<__int128_t>::is_modulo);
		EATEST_VERIFY(eastl::numeric_limits<__uint128_t>::is_modulo);
	#endif
	EATEST_VERIFY(!eastl::numeric_limits<float>::is_modulo);
	EATEST_VERIFY(!eastl::numeric_limits<double>::is_modulo);
	EATEST_VERIFY(!eastl::numeric_limits<long double>::is_modulo);

	// Every numeric_limits member function is noexcept, as in the standard.
	#define EASTL_TEST_NUMERIC_LIMITS_NOEXCEPT(T) \
		static_assert(noexcept(eastl::numeric_limits<T>::min()),           "numeric_limits<" #T ">::min is noexcept"); \
		static_assert(noexcept(eastl::numeric_limits<T>::max()),           "numeric_limits<" #T ">::max is noexcept"); \
		static_assert(noexcept(eastl::numeric_limits<T>::lowest()),        "numeric_limits<" #T ">::lowest is noexcept"); \
		static_assert(noexcept(eastl::numeric_limits<T>::epsilon()),       "numeric_limits<" #T ">::epsilon is noexcept"); \
		static_assert(noexcept(eastl::numeric_limits<T>::round_error()),   "numeric_limits<" #T ">::round_error is noexcept"); \
		static_assert(noexcept(eastl::numeric_limits<T>::infinity()),      "numeric_limits<" #T ">::infinity is noexcept"); \
		static_assert(noexcept(eastl::numeric_limits<T>::quiet_NaN()),     "numeric_limits<" #T ">::quiet_NaN is noexcept"); \
		static_assert(noexcept(eastl::numeric_limits<T>::signaling_NaN()), "numeric_limits<" #T ">::signaling_NaN is noexcept"); \
		static_assert(noexcept(eastl::numeric_limits<T>::denorm_min()),    "numeric_limits<" #T ">::denorm_min is noexcept");

	EASTL_TEST_NUMERIC_LIMITS_NOEXCEPT(bool)
	EASTL_TEST_NUMERIC_LIMITS_NOEXCEPT(char)
	EASTL_TEST_NUMERIC_LIMITS_NOEXCEPT(unsigned char)
	EASTL_TEST_NUMERIC_LIMITS_NOEXCEPT(signed char)
	EASTL_TEST_NUMERIC_LIMITS_NOEXCEPT(wchar_t)
	#if defined(EA_CHAR8_UNIQUE) && EA_CHAR8_UNIQUE
	EASTL_TEST_NUMERIC_LIMITS_NOEXCEPT(char8_t)
	#endif
	EASTL_TEST_NUMERIC_LIMITS_NOEXCEPT(char16_t)
	EASTL_TEST_NUMERIC_LIMITS_NOEXCEPT(char32_t)
	EASTL_TEST_NUMERIC_LIMITS_NOEXCEPT(unsigned short)
	EASTL_TEST_NUMERIC_LIMITS_NOEXCEPT(signed short)
	EASTL_TEST_NUMERIC_LIMITS_NOEXCEPT(unsigned int)
	EASTL_TEST_NUMERIC_LIMITS_NOEXCEPT(signed int)
	EASTL_TEST_NUMERIC_LIMITS_NOEXCEPT(unsigned long)
	EASTL_TEST_NUMERIC_LIMITS_NOEXCEPT(signed long)
	EASTL_TEST_NUMERIC_LIMITS_NOEXCEPT(unsigned long long)
	EASTL_TEST_NUMERIC_LIMITS_NOEXCEPT(signed long long)
	EASTL_TEST_NUMERIC_LIMITS_NOEXCEPT(float)
	EASTL_TEST_NUMERIC_LIMITS_NOEXCEPT(double)
	EASTL_TEST_NUMERIC_LIMITS_NOEXCEPT(long double)
	EASTL_TEST_NUMERIC_LIMITS_NOEXCEPT(const int)
	EASTL_TEST_NUMERIC_LIMITS_NOEXCEPT(volatile double)
	EASTL_TEST_NUMERIC_LIMITS_NOEXCEPT(const volatile float)
	#if (EA_COMPILER_INTMAX_SIZE >= 16) && (defined(EA_COMPILER_GNUC) || defined(EA_COMPILER_CLANG)) // If __int128_t/__uint128_t is supported...
		EASTL_TEST_NUMERIC_LIMITS_NOEXCEPT(__uint128_t)
		EASTL_TEST_NUMERIC_LIMITS_NOEXCEPT(__int128_t)
	#endif
	#undef EASTL_TEST_NUMERIC_LIMITS_NOEXCEPT

	// eastl::isnan (numeric.h) is noexcept as in the standard.
	static_assert(noexcept(eastl::isnan(1.0f)), "isnan(float) is noexcept");
	static_assert(noexcept(eastl::isnan(1.0)), "isnan(double) is noexcept");
	static_assert(noexcept(eastl::isnan(1.0L)), "isnan(long double) is noexcept");

	return nErrorCount;
}












