/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


// Compile-only test: <EASTL/chrono.h> must not include <Windows.h> nor define or undefine the
// user's macros, and <Windows.h> included after it must accept its declarations.
// TestChronoWindowsHBefore.cpp includes <Windows.h> before <EASTL/chrono.h>.

#include <EABase/eabase.h>

#if defined(EA_PLATFORM_WINDOWS)
	#include <EASTL/numeric_limits.h> // #undefs min and max when first included (EASTL_NOMINMAX).

	#define NOMINMAX 1
	#define min(a, b) (((a) < (b)) ? (a) : (b))
	#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#include <EASTL/chrono.h>

#if defined(EA_PLATFORM_WINDOWS)
	#if defined(_WINDOWS_)
		#error <EASTL/chrono.h> included <Windows.h>
	#endif
	#if defined(WIN32_LEAN_AND_MEAN)
		#error <EASTL/chrono.h> defined WIN32_LEAN_AND_MEAN
	#endif
	#if !defined(NOMINMAX) || (NOMINMAX + 0) != 1
		#error <EASTL/chrono.h> changed NOMINMAX
	#endif
	#if !defined(min) || !defined(max)
		#error <EASTL/chrono.h> undefined min or max
	#endif

	#include <Windows.h>

	static_assert(eastl::is_same<decltype(&QueryPerformanceCounter), BOOL (WINAPI*)(LARGE_INTEGER*)>::value, "QueryPerformanceCounter");
	static_assert(eastl::is_same<decltype(&QueryPerformanceFrequency), BOOL (WINAPI*)(LARGE_INTEGER*)>::value, "QueryPerformanceFrequency");
#endif
