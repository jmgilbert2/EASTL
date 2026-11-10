/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_TUPLE_FWD_DECLS_H
#define EASTL_TUPLE_FWD_DECLS_H

#include <EASTL/internal/config.h>

#if EASTL_TUPLE_ENABLED

namespace eastl
{
	template <typename... T>
	class tuple;

	template <typename Tuple>
	struct tuple_size;

#if EASTL_VARIABLE_TEMPLATES_ENABLED
	template <class T>
	EA_CONSTEXPR size_t tuple_size_v = tuple_size<T>::value;
#endif

	template <size_t I, typename Tuple>
	struct tuple_element;

	template <size_t I, typename Tuple>
	using tuple_element_t = typename tuple_element<I, Tuple>::type;

	// get
	template <size_t I, typename... Ts_>
	EA_CPP14_CONSTEXPR tuple_element_t<I, tuple<Ts_...>>& get(tuple<Ts_...>& t) EA_NOEXCEPT;

	template <size_t I, typename... Ts_>
	EA_CPP14_CONSTEXPR const tuple_element_t<I, tuple<Ts_...>>& get(const tuple<Ts_...>& t) EA_NOEXCEPT;

	template <size_t I, typename... Ts_>
	EA_CPP14_CONSTEXPR tuple_element_t<I, tuple<Ts_...>>&& get(tuple<Ts_...>&& t) EA_NOEXCEPT;

	template <size_t I, typename... Ts_>
	EA_CPP14_CONSTEXPR const tuple_element_t<I, tuple<Ts_...>>&& get(const tuple<Ts_...>&& t) EA_NOEXCEPT;

	template <typename T, typename... ts_>
	EA_CPP14_CONSTEXPR T& get(tuple<ts_...>& t) EA_NOEXCEPT;

	template <typename T, typename... ts_>
	EA_CPP14_CONSTEXPR const T& get(const tuple<ts_...>& t) EA_NOEXCEPT;

	template <typename T, typename... ts_>
	EA_CPP14_CONSTEXPR T&& get(tuple<ts_...>&& t) EA_NOEXCEPT;

	template <typename T, typename... ts_>
	EA_CPP14_CONSTEXPR const T&& get(const tuple<ts_...>&& t) EA_NOEXCEPT;
}

#endif  // EASTL_VARIADIC_TEMPLATES_ENABLED

#endif  // EASTL_TUPLE_FWD_DECLS_H
