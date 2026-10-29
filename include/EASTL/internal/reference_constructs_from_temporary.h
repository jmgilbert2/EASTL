///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_INTERNAL_REFERENCE_CONSTRUCTS_FROM_TEMPORARY_H
#define EASTL_INTERNAL_REFERENCE_CONSTRUCTS_FROM_TEMPORARY_H

#include <EASTL/internal/config.h>
#include <EASTL/type_traits.h>

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once
#endif

namespace eastl
{
	namespace Internal
	{
		// Constructor arguments are reference expressions. A conversion can nevertheless create a temporary
		// inside the constructor; in C++23 the corresponding overload must be deleted, rather than removed
		// from overload resolution, so that a const-reference overload cannot silently take its place.
		template <typename T, typename Source, bool = is_reference_v<T>>
		struct ReferenceConstructsFromTemporary : false_type {};

	#if (__cplusplus > 202002L) && (EA_COMPILER_HAS_BUILTIN(__reference_constructs_from_temporary) || \
		(defined(_MSC_VER) && !defined(__clang__) && (_MSC_VER >= 1951)))
		template <typename T, typename Source>
		struct ReferenceConstructsFromTemporary<T, Source, true>
			: bool_constant<__reference_constructs_from_temporary(T, Source)> {};
	#endif
	} // namespace Internal
} // namespace eastl

#endif // EASTL_INTERNAL_REFERENCE_CONSTRUCTS_FROM_TEMPORARY_H
