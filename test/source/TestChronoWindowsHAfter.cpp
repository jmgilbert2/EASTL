/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


// Compile-only test: <EASTL/chrono.h> must not include <Windows.h>, define WIN32_LEAN_AND_MEAN
// or change the user's NOMINMAX. Like numeric_limits.h, it #undefs min and max when
// EASTL_NOMINMAX is 1 (the default). <Windows.h> included after it must compile.
// TestChronoWindowsHBefore.cpp includes <Windows.h> before <EASTL/chrono.h>.

#include <EABase/eabase.h>

#if defined(EA_PLATFORM_WINDOWS)
	#include <EASTL/numeric_limits.h> // First, so that the min/max check below tests chrono.h itself.

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
	#if EASTL_NOMINMAX && (defined(min) || defined(max))
		#error <EASTL/chrono.h> did not #undef min and max (EASTL_NOMINMAX)
	#endif

	#include <Windows.h>
#endif
