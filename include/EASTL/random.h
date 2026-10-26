///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This file defines random number generation like the std C++ <random> header. 
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_RANDOM_H
#define EASTL_RANDOM_H

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once
#endif


#include <EASTL/internal/config.h>
#include <EASTL/numeric_limits.h>

#if defined(EA_COMPILER_MSVC) && !EASTL_GCC_STYLE_INT128_SUPPORTED && (defined(EA_PROCESSOR_X86_64) || defined(EA_PROCESSOR_ARM64))
	#include <intrin.h> // _umul128, __umulh
#endif



///////////////////////////////////////////////////////////////////////////////
// min/max workaround
//
// MSVC++ has #defines for min/max which collide with the min/max algorithm
// declarations. The following may still not completely resolve some kinds of
// problems with MSVC++ #defines, though it deals with most cases in production
// game code.
//
#if EASTL_NOMINMAX
	#ifdef min
		#undef min
	#endif
	#ifdef max
		#undef max
	#endif
#endif


namespace eastl
{

	namespace Internal
	{
		// The largest n where [0, 2^n) fits in [0, range].
		template <typename T>
		constexpr int urng_bit_count(T range)
		{
			int n = (int)(sizeof(T) * 8);
			for(T mask = (T)~T(0); range < mask; mask = (T)(mask >> 1))
				--n;
			return n;
		}

		// urng_bits
		//
		// Draws kBits uniformly distributed random bits per get() from a uniform random bit generator.
		// A standard generator returns values in [URNG::min(), URNG::max()]: like the MSVC STL's
		// _Rng_from_urng_v2, get() uses the largest power-of-two range that fits and rejects the values
		// above it (std::minstd_rand gives 30 bits). A generator without min() and max(), such as
		// EASTLTest_Rand, is assumed to return any value of its result type.
		template <typename URNG, typename = void>
		struct urng_bits
		{
			typedef typename eastl::make_unsigned<typename eastl::decay<decltype(eastl::declval<URNG&>()())>::type>::type result_type;

			static const int kBits = (int)(sizeof(result_type) * 8);

			static result_type get(URNG& urng) { return (result_type)urng(); }
		};

		template <typename URNG>
		struct urng_bits<URNG, eastl::void_t<decltype((URNG::min)()), decltype((URNG::max)())>>
		{
			typedef typename eastl::make_unsigned<typename eastl::decay<decltype(eastl::declval<URNG&>()())>::type>::type result_type;

			static const int         kBits = urng_bit_count((result_type)((URNG::max)() - (URNG::min)()));
			static const result_type kMask = (result_type)((result_type)~result_type(0) >> ((int)(sizeof(result_type) * 8) - kBits));

			static result_type get(URNG& urng)
			{
				result_type value = (result_type)(urng() - (URNG::min)());
				while(value > kMask) // Only when max() - min() + 1 isn't a power of two.
					value = (result_type)(urng() - (URNG::min)());
				return value;
			}
		};

		// Returns sizeof(UInt) * 8 uniformly distributed random bits, from several calls if the generator gives fewer.
		template <typename UInt, typename URNG>
		inline UInt urng_get_bits(URNG& urng)
		{
			typedef urng_bits<URNG> urng_bits_type;
			const int kShift = urng_bits_type::kBits % (int)(sizeof(UInt) * 8); // kBits whenever the loop runs; avoids an oversized shift otherwise.

			UInt value = (UInt)urng_bits_type::get(urng);
			for(int n = urng_bits_type::kBits; n < (int)(sizeof(UInt) * 8); n += urng_bits_type::kBits)
				value = (UInt)((value << kShift) | (UInt)urng_bits_type::get(urng));
			return value;
		}

		// Returns the high half of the 128-bit product a * b and stores its low half in low.
		inline uint64_t urng_mul128(uint64_t a, uint64_t b, uint64_t& low)
		{
		#if EASTL_GCC_STYLE_INT128_SUPPORTED
			const __uint128_t product = (__uint128_t)a * b;
			low = (uint64_t)product;
			return (uint64_t)(product >> 64);
		#elif defined(EA_COMPILER_MSVC) && defined(EA_PROCESSOR_X86_64)
			uint64_t high;
			low = _umul128(a, b, &high);
			return high;
		#elif defined(EA_COMPILER_MSVC) && defined(EA_PROCESSOR_ARM64)
			low = a * b;
			return __umulh(a, b);
		#else
			const uint64_t ll = (a & 0xffffffffu) * (b & 0xffffffffu), lh = (a & 0xffffffffu) * (b >> 32);
			const uint64_t hl = (a >> 32) * (b & 0xffffffffu),         hh = (a >> 32) * (b >> 32);
			const uint64_t middle = (ll >> 32) + (lh & 0xffffffffu) + (hl & 0xffffffffu);
			low = (middle << 32) | (ll & 0xffffffffu);
			return hh + (lh >> 32) + (hl >> 32) + (middle >> 32);
		#endif
		}

		// Whether urng_mul128 is a single multiply on this target.
		#if EASTL_GCC_STYLE_INT128_SUPPORTED || (defined(EA_COMPILER_MSVC) && (defined(EA_PROCESSOR_X86_64) || defined(EA_PROCESSOR_ARM64)))
			const bool kUrngNativeMul128 = true;
		#else
			const bool kUrngNativeMul128 = false;
		#endif

		// Returns a uniformly distributed random value in [0, maxValue]. Like the MSVC STL's _Rng_from_urng_v2, uses
		// Lemire's multiply-shift method with rejection ("Fast Random Integer Generation in an Interval", 2019):
		// one 32-bit draw and a 32x32->64 multiply per value. The division that sets the rejection threshold is
		// only needed when the low half of the product is at most maxValue, which is rare unless the range is wide.
		template <typename URNG>
		inline uint32_t urng_value32(URNG& urng, uint32_t maxValue)
		{
			const uint32_t n = maxValue + 1; // 0 if maxValue is 2^32 - 1.
			const uint32_t bits = urng_get_bits<uint32_t>(urng);
			uint64_t product = (uint64_t)bits * n;

			if((uint32_t)product <= maxValue) // Only then can the low half be below 2^32 % n, the values to reject.
			{
				if(n == 0)
					return bits;

				const uint32_t threshold = (uint32_t)~maxValue % n; // (2^32 - n) % n == 2^32 % n
				while((uint32_t)product < threshold)
					product = (uint64_t)urng_get_bits<uint32_t>(urng) * n;
			}

			return (uint32_t)(product >> 32);
		}

		// As urng_value32, with 64 random bits and a 128-bit product.
		template <typename URNG>
		inline uint64_t urng_value64(URNG& urng, uint64_t maxValue)
		{
			const uint64_t n = maxValue + 1; // 0 if maxValue is 2^64 - 1.
			uint64_t bits = urng_get_bits<uint64_t>(urng);
			uint64_t low;
			uint64_t high = urng_mul128(bits, n, low);

			if(low <= maxValue)
			{
				if(n == 0)
					return bits;

				const uint64_t threshold = ~maxValue % n; // (2^64 - n) % n == 2^64 % n
				while(low < threshold)
				{
					bits = urng_get_bits<uint64_t>(urng);
					high = urng_mul128(bits, n, low);
				}
			}

			return high;
		}

		// The 64-bit method is for ranges above 2^32, and for every range with generators that give 64 bits
		// where the 128-bit product is a single multiply, as in the MSVC STL: it then rarely needs the division.
		template <typename URNG>
		struct urng_use_value64 : public eastl::integral_constant<bool, kUrngNativeMul128 && (urng_bits<URNG>::kBits >= 64)> {};

		template <typename URNG>
		inline uint32_t urng_value(URNG& urng, uint32_t maxValue, eastl::true_type)
			{ return (uint32_t)urng_value64(urng, maxValue); }

		template <typename URNG>
		inline uint32_t urng_value(URNG& urng, uint32_t maxValue, eastl::false_type)
			{ return urng_value32(urng, maxValue); }

		template <typename URNG>
		inline uint64_t urng_value(URNG& urng, uint64_t maxValue, eastl::true_type)
			{ return urng_value64(urng, maxValue); }

		// Ranges above 2^32 with generators that give fewer than 64 bits, or without a native 128-bit product:
		// rare, and out of line so that the code for them doesn't take registers from the common case.
		template <typename URNG>
		EA_NO_INLINE uint64_t urng_value64_wide(URNG& urng, uint64_t maxValue)
			{ return urng_value64(urng, maxValue); }

		template <typename URNG>
		inline uint64_t urng_value(URNG& urng, uint64_t maxValue, eastl::false_type)
			{ return (maxValue <= 0xffffffffu) ? (uint64_t)urng_value32(urng, (uint32_t)maxValue) : urng_value64_wide(urng, maxValue); }

		// Returns a uniformly distributed random value in [0, maxValue].
		template <typename URNG>
		inline uint32_t urng_value(URNG& urng, uint32_t maxValue)
			{ return urng_value(urng, maxValue, urng_use_value64<URNG>()); }

		template <typename URNG>
		inline uint64_t urng_value(URNG& urng, uint64_t maxValue)
			{ return urng_value(urng, maxValue, urng_use_value64<URNG>()); }
	}

	// Implements a uniform distribution of values generated by a Generator, 
	// where Generator is typically a random or pseudo-random number generator.
	// Note that the min/max range for this class is inclusive, so if you want 
	// random integers 0, 1, 2, and 3, then you need to init this class with (0, 3)
	// and not (0, 4).
	// See the C++11 Standard, section 26.5.1.6
	template<class IntType = int>
	class uniform_int_distribution
	{
		static_assert(eastl::is_integral<IntType>::value, "uniform_int_distribution: IntType must be integral.");

	public:
		typedef IntType result_type;

		// For uniform_int_distribution, param_type defines simply the min and max values of
		// the range returned by operator(). It may mean something else for other distribution types.
		struct param_type
		{
			explicit param_type(IntType a = 0, IntType b = eastl::numeric_limits<IntType>::max());

			result_type a() const;
			result_type b() const;

			bool operator==(const param_type& x) const { return (x.mA == mA) && (x.mB == mB); }
			bool operator!=(const param_type& x) const { return (x.mA != mA) || (x.mB != mB); }

		protected:
			IntType mA;
			IntType mB;
		};

		uniform_int_distribution(IntType a = 0, IntType b = eastl::numeric_limits<IntType>::max());
		uniform_int_distribution(const param_type& params);

		void reset();

		template<class Generator>
		result_type operator()(Generator& g);

		template<class Generator>
		result_type operator()(Generator& g, const param_type& params);

		result_type a() const;
		result_type b() const;

		param_type param() const;
		void param(const param_type& params);

		result_type min() const;
		result_type max() const;

		protected:
			param_type mParam;
	};



	///////////////////////////////////////////////////////////////////////
	// uniform_int_distribution
	///////////////////////////////////////////////////////////////////////

	template<class IntType>
	inline uniform_int_distribution<IntType>::param_type::param_type(IntType aValue, IntType bValue)
	  : mA(aValue), mB(bValue)
	{
		EASTL_ASSERT(aValue <= bValue);
	}

	template<class IntType>
	inline typename uniform_int_distribution<IntType>::result_type 
	uniform_int_distribution<IntType>::param_type::a() const
	{
		return mA;
	}

	template<class IntType>
	inline typename uniform_int_distribution<IntType>::result_type 
	uniform_int_distribution<IntType>::param_type::b() const
	{
		return mB;
	}



	template<class IntType>
	inline uniform_int_distribution<IntType>::uniform_int_distribution(IntType aValue, IntType bValue) 
	  : mParam(aValue, bValue)
	{
		// Nothing more to do.
	}

	template<class IntType>
	inline uniform_int_distribution<IntType>::uniform_int_distribution(const param_type& params) 
	  : mParam(params)
	{
		// Nothing more to do.
	}

	template<class IntType>
	void uniform_int_distribution<IntType>::reset()
	{
		// Nothing to do.
	}

	template<class IntType>
	template<class Generator>
	inline typename uniform_int_distribution<IntType>::result_type 
	uniform_int_distribution<IntType>::operator()(Generator& g)
	{
		return operator()(g, mParam);
	}

	template<class IntType>
	template<class Generator>
	inline typename uniform_int_distribution<IntType>::result_type 
	uniform_int_distribution<IntType>::operator()(Generator& g, const param_type& params)
	{
		static_assert(sizeof(result_type) <= 8, "uniform_int_distribution supports integer types of up to 64 bits.");

		// We draw a value in [0, b - a] in the unsigned type of at least 32 bits that holds b - a, as shuffle
		// draws its indexes, and add it to a in the unsigned type of IntType.
		typedef typename eastl::make_unsigned<result_type>::type                                   unsigned_type;
		typedef typename eastl::conditional<(sizeof(result_type) <= 4), uint32_t, uint64_t>::type range_type;

		const unsigned_type a     = (unsigned_type)params.a(); // Read before calling g, so that it needn't be reloaded after.
		const range_type    range = (range_type)(unsigned_type)((unsigned_type)params.b() - a);

		return (result_type)(unsigned_type)(a + (unsigned_type)Internal::urng_value(g, range));
	}

	template<class IntType>
	inline typename uniform_int_distribution<IntType>::result_type
	uniform_int_distribution<IntType>::a() const
	{
		return mParam.a();
	}

	template<class IntType>
	inline typename uniform_int_distribution<IntType>::result_type
	uniform_int_distribution<IntType>::b() const
	{
		return mParam.b();
	}


	template<class IntType>
	inline typename uniform_int_distribution<IntType>::param_type
	uniform_int_distribution<IntType>::param() const
	{
		return mParam;
	}

	template<class IntType>
	inline void
	uniform_int_distribution<IntType>::param(const param_type& params)
	{
		mParam = params;
	}

	template<class IntType>
	inline typename uniform_int_distribution<IntType>::result_type
	uniform_int_distribution<IntType>::min() const
	{
		return mParam.a();
	}

	template<class IntType>
	inline typename uniform_int_distribution<IntType>::result_type
	uniform_int_distribution<IntType>::max() const
	{
		return mParam.b();
	}



	template<class ResultType>
	inline bool operator==(const uniform_int_distribution<ResultType>& lhs,
						   const uniform_int_distribution<ResultType>& rhs)
	{
		return (lhs.param() == rhs.param());
	}

	template<class ResultType>
	inline bool operator!=(const uniform_int_distribution<ResultType>& lhs,
						   const uniform_int_distribution<ResultType>& rhs)
	{
		return (lhs.param() != rhs.param());
	}


	// EASTL doesn't currently implement IO stream-related functionality.
	// It may be useful to forward declare these templates and let the user implement them in the meantime.
	//
	// template<class CharT, class Traits, class ResultType>
	// eastl::basic_ostream<CharT, Traits>& operator<<(eastl::basic_ostream<CharT, Traits>& os, const uniform_int_distribution& uid);
	//
	// template<class CharT, class Traits, class ResultType>
	// eastl::basic_istream<CharT, Traits>& operator>>(eastl::basic_istream<CharT, Traits>& is, uniform_int_distribution& uid);


} // namespace eastl


#endif // Header include guard







