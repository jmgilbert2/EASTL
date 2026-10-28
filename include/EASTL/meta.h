/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_META_H
#define EASTL_META_H

#include <EASTL/internal/config.h>
#include <EASTL/type_traits.h>
#include <EASTL/internal/integer_sequence.h>

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif

////////////////////////////////////////////////////////////////////////////////////////////
// This file contains meta programming utilities that are internal to EASTL. We reserve
// the right to change this file at any time as it is only intended to be used internally.
////////////////////////////////////////////////////////////////////////////////////////////

namespace eastl
{
	namespace meta
	{
		////////////////////////////////////////////////////////////////////////////////////////////
		// get_type_index_v
		//
		// Linearly searches a typelist using compile-time recursion to inspect each T in
		// the typelist and returns its index, if the type is found.  If the T isn't found
		// in the typelist -1 is returned.
		//
		namespace Internal
		{
			template <int I, typename T, typename... Types>
			struct get_type_index;

			template <int I, typename T, typename Head, typename... Types>
			struct get_type_index<I, T, Head, Types...>
			{
				static EA_CONSTEXPR_OR_CONST int value = is_same_v<T, Head> ? I : get_type_index<I + 1, T, Types...>::value;
			};

			template <int I, typename T>
			struct get_type_index<I, T>
			{
				static EA_CONSTEXPR_OR_CONST int value = -1;
			};
		}

		template<typename T, typename... Types>
		struct get_type_index
		{
			static EA_CONSTEXPR_OR_CONST int value = Internal::get_type_index<0, T, Types...>::value;
		};

		template <typename T, typename... Ts>
		EASTL_CPP17_INLINE_VARIABLE EA_CONSTEXPR int get_type_index_v = get_type_index<T, Ts...>::value;


		////////////////////////////////////////////////////////////////////////////////////////////
		// get_type_at
		//
		// This traverses the variadic type list and retrieves the type at the user provided index.
		//
		template <size_t I, typename... Ts>
			struct get_type_at_helper;

		template <size_t I, typename Head, typename... Tail>
			struct get_type_at_helper<I, Head, Tail...>
			{ typedef typename get_type_at_helper<I - 1, Tail...>::type type; };

		template <typename Head, typename... Tail>
			struct get_type_at_helper<0, Head, Tail...>
			{ typedef Head type; };

		template <int I, typename... Ts>
		using get_type_at_t = typename get_type_at_helper<I, Ts...>::type;


		////////////////////////////////////////////////////////////////////////////////////////////
		// type_count_v
		//
		// Returns the number of occurrences of type T in a typelist.
		//
		template <typename T, typename... Types>
		struct type_count;

		template <typename T, typename H, typename... Types>
		struct type_count<T, H, Types...>
		{
			static EA_CONSTEXPR_OR_CONST int value = (is_same_v<T, H> ? 1 : 0) + type_count<T, Types...>::value;
		};

		template <typename T>
		struct type_count<T>
		{
			static EA_CONSTEXPR_OR_CONST int value = 0;
		};

		template <typename T, typename... Ts>
		EASTL_CPP17_INLINE_VARIABLE EA_CONSTEXPR int type_count_v = type_count<T, Ts...>::value;



		////////////////////////////////////////////////////////////////////////////////////////////
		// duplicate_type_check_v
		//
		// Checks if a type T occurs in a typelist more than once.
		//
		template <typename T, typename... Types>
		struct duplicate_type_check
		{
			static EA_CONSTEXPR_OR_CONST bool value = (type_count<T, Types...>::value == 1);
		};

		template <typename... Ts>
		EASTL_CPP17_INLINE_VARIABLE EA_CONSTEXPR bool duplicate_type_check_v = duplicate_type_check<Ts...>::value;


		//////////////////////////////////////////////////////////////////////////////////
		// type_list
		//
		// type_list is a simple struct that allows us to pass template parameter packs
		// around in a single struct, and deduce parameter packs from function arguments
		// like so:
		//
		//     template <typename... Ts> void foo(type_list<Ts...>);
		//     foo(type_list<A, B, C>); // deduces Ts... as A, B, C
		//
		template <typename...> struct type_list {};


		////////////////////////////////////////////////////////////////////////////////////////////
		// double_pack_expansion
		//
		// MSVC 2017 has a hard time expanding two packs of different lengths.
		// This is a helper meant to coerce MSVC 2017 into doing the expansion by adding another level
		// of indirection.
		//

		template <typename T, size_t I>
		struct double_pack_expansion;

		template <size_t... Is, size_t I>
		struct double_pack_expansion<index_sequence<Is...>, I>
		{
			using type = index_sequence<Is..., I>;
		};

		template <typename IndexSequence, size_t I>
		using double_pack_expansion_t = typename double_pack_expansion<IndexSequence, I>::type;


	} // namespace meta
} // namespace eastl

#endif // EASTL_META_H
