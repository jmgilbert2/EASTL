/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


// Compile-only test: <EASTL/vector.h>, then <Windows.h> without NOMINMAX or WIN32_LEAN_AND_MEAN,
// then <EASTL/chrono.h>. <EASTL/chrono.h> must compile with the min and max macros <Windows.h>
// defines (it #undefs them when EASTL_NOMINMAX is 1, the default, like vector.h) and must not
// define NOMINMAX or WIN32_LEAN_AND_MEAN.
// TestChronoWindowsHAfter.cpp includes <Windows.h> after <EASTL/chrono.h>.

#include <EABase/eabase.h>

#if defined(EA_PLATFORM_WINDOWS)
	#include <EASTL/vector.h> // #undefs min and max (EASTL_NOMINMAX) before <Windows.h> defines them.
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
	#if EASTL_NOMINMAX && (defined(min) || defined(max))
		#error <EASTL/chrono.h> did not #undef min and max (EASTL_NOMINMAX)
	#endif
#endif
