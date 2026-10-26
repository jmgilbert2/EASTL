///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include <EASTL/chrono.h>

#if defined(EA_PLATFORM_MICROSOFT)
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <Windows.h>


namespace eastl
{
namespace chrono
{
	namespace Internal
	{
		///////////////////////////////////////////////////////////////////////////////
		// Internal::GetTicks
		///////////////////////////////////////////////////////////////////////////////
		EASTL_API uint64_t GetTicks()
		{
			auto queryFrequency = []
			{
				LARGE_INTEGER frequency;
				QueryPerformanceFrequency(&frequency);
				return double(1000000000.0L / (long double)frequency.QuadPart);  // nanoseconds per tick
			};

			auto queryCounter = []
			{
				LARGE_INTEGER counter;
				QueryPerformanceCounter(&counter);
				return counter.QuadPart;
			};

			EA_DISABLE_VC_WARNING(4640)  // warning C4640: construction of local static object is not thread-safe (VS2013)
			static auto frequency = queryFrequency(); // cache cpu frequency on first call
			EA_RESTORE_VC_WARNING()
			return uint64_t(frequency * (double)queryCounter());
		}
	} // namespace Internal
} // namespace chrono
} // namespace eastl

#endif // EA_PLATFORM_MICROSOFT
