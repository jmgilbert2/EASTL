/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#include "EASTLTest.h"
#include <EABase/eabase.h>
#include <EASTL/chrono.h>
#include <EASTL/numeric.h>
#include <EASTL/string.h>

// The exception specifications of duration, time_point and the chrono literals follow the standard: the arithmetic and
// the comparisons are noexcept when the representation is an arithmetic type, as MSVC's STL strengthens them.
namespace NoexceptChronoTest
{
	// A class type, which is not arithmetic, whose copy doesn't throw.
	struct ClassRep
	{
		ClassRep() EA_NOEXCEPT; ClassRep(int) EA_NOEXCEPT; ClassRep(const ClassRep&) EA_NOEXCEPT; ClassRep& operator=(const ClassRep&) EA_NOEXCEPT;
		ClassRep& operator++(); ClassRep operator++(int); ClassRep& operator--(); ClassRep operator--(int);
		ClassRep& operator+=(const ClassRep&); ClassRep& operator-=(const ClassRep&); ClassRep& operator*=(const ClassRep&);
		ClassRep& operator/=(const ClassRep&); ClassRep& operator%=(const ClassRep&);
		ClassRep operator-() const;
	};

	typedef eastl::chrono::duration<int>                       IntDuration;
	typedef eastl::chrono::duration<long long, eastl::milli>   MilliDuration;
	typedef eastl::chrono::duration<double>                    DoubleDuration;
	typedef eastl::chrono::duration<ClassRep>                  ClassDuration;
	typedef eastl::chrono::time_point<eastl::chrono::system_clock, MilliDuration> MilliTimePoint;
	typedef eastl::chrono::time_point<eastl::chrono::system_clock, eastl::chrono::seconds> SecondsTimePoint;
	typedef eastl::chrono::time_point<eastl::chrono::system_clock, ClassDuration> ClassTimePoint;

	// duration_values and the static members.
	static_assert(noexcept(eastl::chrono::duration_values<int>::zero()) && noexcept(eastl::chrono::duration_values<int>::max()) && noexcept(eastl::chrono::duration_values<int>::min()), "duration_values is noexcept");
	static_assert(noexcept(IntDuration::zero()) && noexcept(IntDuration::min()) && noexcept(IntDuration::max()), "duration::zero, min and max are noexcept");
	static_assert(noexcept(MilliTimePoint::min()) && noexcept(MilliTimePoint::max()), "time_point::min and max are noexcept");

	// The conditional ones, for arithmetic and class representations.
	#define EASTL_TEST_NOEXCEPT_DURATION(D, Expected) \
		static_assert(noexcept(eastl::declval<const D&>().count()) == Expected, "duration::count exception specification"); \
		static_assert(noexcept(+eastl::declval<const D&>()) == Expected, "duration::operator+ exception specification"); \
		static_assert(noexcept(-eastl::declval<const D&>()) == Expected, "duration::operator- exception specification"); \
		static_assert(noexcept(eastl::declval<D&>()++) == Expected, "duration::operator++(int) exception specification"); \
		static_assert(noexcept(eastl::declval<D&>()--) == Expected, "duration::operator--(int) exception specification"); \
		static_assert(noexcept(++eastl::declval<D&>()) == Expected, "duration::operator++ exception specification"); \
		static_assert(noexcept(--eastl::declval<D&>()) == Expected, "duration::operator-- exception specification"); \
		static_assert(noexcept(eastl::declval<D&>() += eastl::declval<const D&>()) == Expected, "duration::operator+= exception specification"); \
		static_assert(noexcept(eastl::declval<D&>() -= eastl::declval<const D&>()) == Expected, "duration::operator-= exception specification"); \
		static_assert(noexcept(eastl::declval<D&>() *= eastl::declval<const typename D::rep&>()) == Expected, "duration::operator*= exception specification"); \
		static_assert(noexcept(eastl::declval<D&>() /= eastl::declval<const typename D::rep&>()) == Expected, "duration::operator/= exception specification"); \
		static_assert(noexcept(eastl::declval<D&>() %= eastl::declval<const D&>()) == Expected, "duration::operator%= exception specification"); \
		static_assert(noexcept(eastl::declval<const D&>() + eastl::declval<const D&>()) == Expected, "duration + duration exception specification"); \
		static_assert(noexcept(eastl::declval<const D&>() - eastl::declval<const D&>()) == Expected, "duration - duration exception specification"); \
		static_assert(noexcept(eastl::declval<const D&>() == eastl::declval<const D&>()) == Expected, "duration == exception specification"); \
		static_assert(noexcept(eastl::declval<const D&>() != eastl::declval<const D&>()) == Expected, "duration != exception specification"); \
		static_assert(noexcept(eastl::declval<const D&>() < eastl::declval<const D&>()) == Expected, "duration < exception specification"); \
		static_assert(noexcept(eastl::declval<const D&>() <= eastl::declval<const D&>()) == Expected, "duration <= exception specification"); \
		static_assert(noexcept(eastl::declval<const D&>() > eastl::declval<const D&>()) == Expected, "duration > exception specification"); \
		static_assert(noexcept(eastl::declval<const D&>() >= eastl::declval<const D&>()) == Expected, "duration >= exception specification");
	EASTL_TEST_NOEXCEPT_DURATION(IntDuration, true)
	EASTL_TEST_NOEXCEPT_DURATION(MilliDuration, true)
	EASTL_TEST_NOEXCEPT_DURATION(DoubleDuration, true)
	EASTL_TEST_NOEXCEPT_DURATION(ClassDuration, false)
	#undef EASTL_TEST_NOEXCEPT_DURATION

	// Operations that mix representations and periods.
	static_assert(noexcept(MilliDuration(eastl::declval<const eastl::chrono::seconds&>())), "duration(const duration<Rep2, Period2>&) is noexcept for arithmetic representations");
	static_assert(noexcept(MilliDuration(eastl::declval<const int&>())), "duration(const Rep2&) is noexcept for arithmetic representations");
	static_assert(noexcept(eastl::declval<const eastl::chrono::seconds&>() + eastl::declval<const MilliDuration&>()), "duration + duration of different types is noexcept");
	static_assert(noexcept(eastl::declval<const eastl::chrono::seconds&>() * 2), "duration * rep is noexcept");
	static_assert(noexcept(2 * eastl::declval<const eastl::chrono::seconds&>()), "rep * duration is noexcept");
	static_assert(noexcept(eastl::declval<const eastl::chrono::seconds&>() / 2), "duration / rep is noexcept");
	static_assert(noexcept(eastl::declval<const eastl::chrono::seconds&>() / eastl::declval<const MilliDuration&>()), "duration / duration is noexcept");
	static_assert(noexcept(eastl::declval<const eastl::chrono::seconds&>() % 2), "duration % rep is noexcept");
	static_assert(noexcept(eastl::declval<const eastl::chrono::seconds&>() % eastl::declval<const MilliDuration&>()), "duration % duration is noexcept");
	static_assert(noexcept(eastl::declval<const eastl::chrono::seconds&>() < eastl::declval<const MilliDuration&>()), "duration < duration of different types is noexcept");
	static_assert(noexcept(eastl::chrono::duration_cast<eastl::chrono::seconds>(eastl::declval<const MilliDuration&>())), "duration_cast is noexcept for arithmetic representations");
	static_assert(noexcept(eastl::chrono::duration_cast<DoubleDuration>(eastl::declval<const MilliDuration&>())), "duration_cast to a floating point duration is noexcept");
	static_assert(!noexcept(eastl::chrono::duration_cast<ClassDuration>(eastl::declval<const ClassDuration&>())), "duration_cast may throw for a class representation");

	// time_point.
	static_assert(noexcept(MilliTimePoint()), "time_point() is noexcept");
	static_assert(noexcept(MilliTimePoint(eastl::declval<const MilliDuration&>())), "time_point(const Duration&) is noexcept for arithmetic representations");
	static_assert(!noexcept(ClassTimePoint(eastl::declval<const ClassDuration&>())), "time_point(const Duration&) may throw for a class representation");
	static_assert(noexcept(MilliTimePoint(eastl::declval<const SecondsTimePoint&>())), "time_point(const time_point<Clock, Duration2>&) is noexcept for arithmetic representations");
	static_assert(noexcept(eastl::declval<const MilliTimePoint&>().time_since_epoch()), "time_point::time_since_epoch is noexcept for arithmetic representations");
	static_assert(!noexcept(eastl::declval<const ClassTimePoint&>().time_since_epoch()), "time_point::time_since_epoch may throw for a class representation");
	static_assert(noexcept(eastl::declval<MilliTimePoint&>() += eastl::declval<const MilliDuration&>()), "time_point::operator+= is noexcept for arithmetic representations");
	static_assert(noexcept(eastl::declval<MilliTimePoint&>() -= eastl::declval<const MilliDuration&>()), "time_point::operator-= is noexcept for arithmetic representations");
	static_assert(!noexcept(eastl::declval<ClassTimePoint&>() += eastl::declval<const ClassDuration&>()), "time_point::operator+= may throw for a class representation");
	static_assert(noexcept(eastl::declval<const MilliTimePoint&>() + eastl::declval<const eastl::chrono::seconds&>()), "time_point + duration is noexcept for arithmetic representations");
	static_assert(noexcept(eastl::declval<const eastl::chrono::seconds&>() + eastl::declval<const MilliTimePoint&>()), "duration + time_point is noexcept for arithmetic representations");
	static_assert(noexcept(eastl::declval<const MilliTimePoint&>() - eastl::declval<const eastl::chrono::seconds&>()), "time_point - duration is noexcept for arithmetic representations");
	static_assert(noexcept(eastl::declval<const MilliTimePoint&>() - eastl::declval<const SecondsTimePoint&>()), "time_point - time_point is noexcept for arithmetic representations");
	static_assert(noexcept(eastl::declval<const MilliTimePoint&>() == eastl::declval<const SecondsTimePoint&>()), "time_point == is noexcept for arithmetic representations");
	static_assert(noexcept(eastl::declval<const MilliTimePoint&>() != eastl::declval<const SecondsTimePoint&>()), "time_point != is noexcept for arithmetic representations");
	static_assert(noexcept(eastl::declval<const MilliTimePoint&>() < eastl::declval<const SecondsTimePoint&>()), "time_point < is noexcept for arithmetic representations");
	static_assert(noexcept(eastl::declval<const MilliTimePoint&>() <= eastl::declval<const SecondsTimePoint&>()), "time_point <= is noexcept for arithmetic representations");
	static_assert(noexcept(eastl::declval<const MilliTimePoint&>() > eastl::declval<const SecondsTimePoint&>()), "time_point > is noexcept for arithmetic representations");
	static_assert(noexcept(eastl::declval<const MilliTimePoint&>() >= eastl::declval<const SecondsTimePoint&>()), "time_point >= is noexcept for arithmetic representations");
	static_assert(noexcept(eastl::chrono::time_point_cast<eastl::chrono::seconds>(eastl::declval<const MilliTimePoint&>())), "time_point_cast is noexcept for arithmetic representations");

	#if EASTL_USER_LITERALS_ENABLED && EASTL_INLINE_NAMESPACES_ENABLED
		using namespace eastl::literals::chrono_literals;
		static_assert(noexcept(1h) && noexcept(1min) && noexcept(1s) && noexcept(1ms) && noexcept(1us) && noexcept(1ns), "the integer chrono literals are noexcept");
		static_assert(noexcept(1.0h) && noexcept(1.0min) && noexcept(1.0s) && noexcept(1.0ms) && noexcept(1.0us) && noexcept(1.0ns), "the floating point chrono literals are noexcept");
	#endif
}


using namespace eastl;
using namespace eastl::chrono;


//////////////////////////////////////////////////////////////////////////////////////////////////
// TestDuration
//
int TestDuration()
{
	int nErrorCount = 0;

	{
		hours h{1}; // 1 hour
		milliseconds ms{3}; // 3 milliseconds
		duration<int, kilo> ks{3}; // 3000 seconds

		duration<double, ratio<1, 30>> hz30{3.5};
		microseconds us = ms; 
		duration<double, milli> ms2 = us; // 3.0 milliseconds

		EA_UNUSED(h);
		EA_UNUSED(ms2);
		EA_UNUSED(ks);
		EA_UNUSED(hz30);
		EA_UNUSED(us);
	}

	{
		typedef duration<double, ratio<1, 30>> dur_t;
		VERIFY(dur_t::min()  < dur_t::zero());
		VERIFY(dur_t::zero() < dur_t::max());
		VERIFY(dur_t::min()  < dur_t::max());
	}

	{
		seconds s1(10);
		seconds s2 = -s1;
		VERIFY(s1.count() ==  10);
		VERIFY(s2.count() == -10);
	}

	{
		{
			hours h(1);
			minutes m = ++h;
			m--;
			VERIFY(m.count() == 119);
		}

		{
			hours h(24);
			minutes m = h;
			seconds s = m; 
			milliseconds ms = s; 

			VERIFY(h.count()  == 24);
			VERIFY(m.count()  == 1440);
			VERIFY(s.count()  == 86400);
			VERIFY(ms.count() == 86400000);
		}

		{    
			minutes m(11);
			m *= 2;
			VERIFY(m.count() == 22);
			m += hours(10); 
			VERIFY(m.count() == 622);
			VERIFY(duration_cast<hours>(m).count() == 10);
			m %= hours(1);
			VERIFY(duration_cast<hours>(m).count() == 0);
			VERIFY(m.count() == 22);
		}

		{
			milliseconds ms(3);       // 3 milliseconds
			VERIFY(ms.count() == 3);

			microseconds us = 2 * ms; // 6000 microseconds constructed from 3 milliseconds
			VERIFY(us.count() == 6000);
			
			microseconds us2 = ms * 2; // 6000 microseconds constructed from 3 milliseconds
			VERIFY(us2.count() == 6000);

			microseconds us3 = us / 2;
			VERIFY(us3.count() == 3000);

			microseconds us4 = us % 2;
			VERIFY(us4.count() == 0);
		}
	}

	{
		seconds s(5);
		milliseconds ms(10);
		VERIFY(s / ms == 500);
		VERIFY(ms / s.count() == milliseconds(2));
	}

	return nErrorCount;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
// TestTimePoint
//
int TestTimePoint()
{
	int nErrorCount = 0;
	{
		{
			system_clock::time_point t0 = system_clock::now();
			auto tomorrow = t0 + hours(24);
			auto today = tomorrow - system_clock::now();
			auto hours_count = duration_cast<hours>(today).count();

			VERIFY(hours_count == 24 || hours_count == 23);  // account for time flux
		}

		{
			time_point<system_clock, hours> hour1(hours(1));

			auto hour_to_min 	  = time_point_cast<minutes>(hour1);
			auto hour_to_sec      = time_point_cast<seconds>(hour1);
			auto hour_to_millisec = time_point_cast<milliseconds>(hour1);
			auto hour_to_microsec = time_point_cast<microseconds>(hour1);
			auto hour_to_nanosec  = time_point_cast<nanoseconds>(hour1);
		
			VERIFY(hour_to_min.time_since_epoch().count()      == 60);
			VERIFY(hour_to_sec.time_since_epoch().count()      == 3600);
			VERIFY(hour_to_millisec.time_since_epoch().count() == 3600000ll);
			VERIFY(hour_to_microsec.time_since_epoch().count() == 3600000000ll);
			VERIFY(hour_to_nanosec.time_since_epoch().count()  == 3600000000000ll);
		}
	}
	return nErrorCount;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
// TestClocks
//
int TestClocks()
{
	int nErrorCount = 0;
	{
		{
			auto sys = system_clock::now();
			VERIFY(sys.time_since_epoch().count() > 0);

			auto stdy = steady_clock::now();
			VERIFY(stdy.time_since_epoch().count() > 0);

			auto hrc = high_resolution_clock::now();
			VERIFY(hrc.time_since_epoch().count() > 0);
		}

		{
			auto start = system_clock::now();
			auto end   = system_clock::now();
			auto d = end - start;
			EA_UNUSED(d);
			VERIFY(d.count() >= 0);
		}

		{
			auto start = steady_clock::now();
			auto end   = steady_clock::now();
			auto d = end - start;
			EA_UNUSED(d);
			VERIFY(d.count() >= 0);
		}

		{
			auto start = high_resolution_clock::now();
			auto end   = high_resolution_clock::now();
			auto d = end - start;
			EA_UNUSED(d);
			VERIFY(d.count() >= 0);
		}

		{
			typedef duration<int, ratio<1, 100000000>> shakes;
			typedef duration<int, centi> jiffies;
			typedef duration<float, ratio<12096, 10000>> microfortnights;
			typedef duration<float, ratio<3155, 1000>> nanocenturies;

			seconds sec(1);

			VERIFY(duration_cast<shakes>(sec).count() == 100000000);
			VERIFY(duration_cast<jiffies>(sec).count() == 100);
			VERIFY(microfortnights(sec).count() > 0.82f);
			VERIFY(nanocenturies(sec).count() > 0.31f);
		}
	}
	return nErrorCount;
}


int TestChrono()
{
	int nErrorCount = 0;
	nErrorCount += TestDuration();
	nErrorCount += TestTimePoint();
	nErrorCount += TestClocks();
	return nErrorCount;
}









