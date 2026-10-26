/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


// Compile-only test: <Windows.h> included before <EASTL/chrono.h>, without NOMINMAX or
// WIN32_LEAN_AND_MEAN. <EASTL/chrono.h> must compile with the min and max macros it defines
// and leave them, and its declarations must match <Windows.h>'s.
// TestChronoWindowsHAfter.cpp includes <Windows.h> after <EASTL/chrono.h>.

#include <EABase/eabase.h>

#if defined(EA_PLATFORM_WINDOWS)
	#include <EASTL/numeric_limits.h> // #undefs min and max when first included (EASTL_NOMINMAX).
	#include <Windows.h>
#endif

#include <EASTL/chrono.h>

#if defined(EA_PLATFORM_WINDOWS)
	#if defined(WIN32_LEAN_AND_MEAN)
		#error <EASTL/chrono.h> defined WIN32_LEAN_AND_MEAN
	#endif
	#if defined(NOMINMAX)
		#error <EASTL/chrono.h> defined NOMINMAX
	#endif
	#if !defined(min) || !defined(max)
		#error <EASTL/chrono.h> undefined min or max
	#endif

	static_assert(eastl::is_same<decltype(&QueryPerformanceCounter), BOOL (WINAPI*)(LARGE_INTEGER*)>::value, "QueryPerformanceCounter");
	static_assert(eastl::is_same<decltype(&QueryPerformanceFrequency), BOOL (WINAPI*)(LARGE_INTEGER*)>::value, "QueryPerformanceFrequency");
#endif
