/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#include "EASTLTest.h"
#include <EASTL/bit.h>

using namespace eastl;


int TestBitcast()
{
	int nErrorCount = 0;

	{
		uint32_t int32Value = 0x12345678;
		float floatValue = eastl::bit_cast<float>(int32Value);
		VERIFY(memcmp(&int32Value, &floatValue, sizeof(float)) == 0);
	}

	{
		struct IntFloatStruct
		{
			uint32_t i = 0x87654321;
			float f = 10.f;
		};
		struct CharIntStruct
		{
			char c1;
			char c2;
			char c3;
			char c4;
			uint32_t i;
		};

		IntFloatStruct ifStruct;
		CharIntStruct ciStruct = eastl::bit_cast<CharIntStruct>(ifStruct);
		VERIFY(memcmp(&ifStruct, &ciStruct, sizeof(IntFloatStruct)) == 0);
	}

#if defined(EA_COMPILER_MSVC) // cl (since VS2019 16.6) and clang-cl both have __builtin_bit_cast.
	static_assert(EASTL_CONSTEXPR_BIT_CAST_SUPPORTED, "eastl::bit_cast should be constexpr");
#endif

#if EASTL_CONSTEXPR_BIT_CAST_SUPPORTED
	{
		constexpr uint32_t int32Value = 40;
		constexpr float floatValue = eastl::bit_cast<float>(int32Value);
		VERIFY(memcmp(&int32Value, &floatValue, sizeof(float)) == 0);

		static_assert(eastl::bit_cast<uint32_t>(1.0f) == UINT32_C(0x3f800000), "bit_cast<uint32_t>(1.0f)");
	}
#endif


	return nErrorCount;
}
