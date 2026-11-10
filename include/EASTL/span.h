/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This file implements the eastl::span which is part of the C++ standard
// STL library specification.
//
// eastl::span is a non-owning container that refers to a contiguous block of
// memory.  It bundles up the classic pattern of a pointer and a size into a
// single type.  A span can either have a static extent, in which case the
// number of elements in the sequence is known and encoded in the type, or a
// dynamic extent.
//
// http://en.cppreference.com/w/cpp/container/span
// http://eel.is/c++draft/views#span.syn
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_SPAN_H
#define EASTL_SPAN_H

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once 
#endif

#include <EASTL/internal/config.h>
#include <EASTL/type_traits.h>
#include <EASTL/iterator.h>
#include <EASTL/array.h>

#include <cstddef> // std::byte, for as_bytes and as_writable_bytes

namespace eastl
{
	static EA_CONSTEXPR size_t dynamic_extent = size_t(-1);

	template <typename T, size_t Extent>
	class span;

	namespace Internal
	{
		// HasSizeAndData
		// 
		// custom type trait to determine if eastl::data(Container) and eastl::size(Container) are well-formed.
		//
		template <typename, typename = void>
		struct HasSizeAndData : eastl::false_type {};

		template <typename T>
		struct HasSizeAndData<T, void_t<decltype(eastl::size(eastl::declval<T>())), decltype(eastl::data(eastl::declval<T>()))>> : eastl::true_type {};

		// SubspanExtent
		//
		// Integral constant that calculates the resulting extent of a templated subspan operation.
		//
		//   If Count is not dynamic_extent then SubspanExtent::value is Count,
		//   otherwise, if Extent is not dynamic_extent, SubspanExtent::value is (Extent - Offset),
		//   otherwise, SubspanExtent::value is dynamic_extent.
		//
		template<size_t Extent, size_t Offset, size_t Count>
		struct SubspanExtent : eastl::integral_constant<size_t, (Count != dynamic_extent ? Count : (Extent != dynamic_extent ? (Extent - Offset) : dynamic_extent))> {};

		// is_eastl_array<PossiblyArray, T>
		// 
		// is an eastl::array<T, N> with elements T?
		// NOT the same as is_array, which is a type trait for the C-style array type.
		template<typename PossiblyArray, typename T>
		struct is_eastl_array : public eastl::false_type {};

		template<typename T, size_t N>
		struct is_eastl_array<eastl::array<T, N>, T> : public eastl::true_type {};

		// is_any_eastl_array<T>, is_eastl_span<T>: an eastl::array or eastl::span of any element type, which the
		// generic container constructors leave to the array and span constructors, as std does.
		template<typename T>
		struct is_any_eastl_array : public eastl::false_type {};

		template<typename T, size_t N>
		struct is_any_eastl_array<eastl::array<T, N>> : public eastl::true_type {};

		template<typename T>
		struct is_eastl_span : public eastl::false_type {};

		template<typename T, size_t Extent>
		struct is_eastl_span<span<T, Extent>> : public eastl::true_type {};

		// SpanStorage
		//
		// Holds all of the member variables for span, specialized to remove the size variable when
		// given a static extent.
		//
		template <typename T, size_t Extent>
		struct SpanStorage
		{
			T* mpData = nullptr;
			static EA_CONSTEXPR eastl_size_t mnSize = Extent;

			EA_CONSTEXPR SpanStorage() EA_NOEXCEPT
			{
				static_assert(Extent == 0, "impossible to default construct a span with a fixed Extent different than 0");
			}

			EA_CONSTEXPR SpanStorage(T* ptr, eastl_size_t size)
				: mpData(ptr)
			{
				EA_UNUSED(size);
				EASTL_ASSERT_MSG(Extent == size, "impossible to create a span with a fixed Extent different than the size of the supplied buffer");
			}
		};

		template <typename T>
		struct SpanStorage<T, eastl::dynamic_extent>
		{
			T* mpData = nullptr;
			eastl_size_t mnSize = 0;

			EA_CONSTEXPR SpanStorage() EA_NOEXCEPT = default;
			EA_CONSTEXPR SpanStorage(T* ptr, eastl_size_t size) : mpData(ptr), mnSize(size) {}
		};
	}

	template <typename T, size_t Extent = eastl::dynamic_extent>
	class span
	{
	public:
		typedef T                                       element_type;
		typedef remove_cv_t<T>                          value_type;
		typedef eastl_size_t                            index_type;
		typedef ptrdiff_t                               difference_type;
		typedef T*                                      pointer;
		typedef const T*                                const_pointer;
		typedef T&                                      reference;
		typedef const T&                                const_reference;
		typedef T*                                      iterator;
		typedef const T*                                const_iterator;
		typedef eastl::reverse_iterator<iterator>       reverse_iterator;
		typedef eastl::reverse_iterator<const_iterator> const_reverse_iterator;

		static EA_CONSTEXPR size_t extent = Extent;

		// constructors / destructor
		EA_CONSTEXPR span() EA_NOEXCEPT = default;
		EA_CONSTEXPR span(const span& other) EA_NOEXCEPT = default;
		// Like std, the constructors from a pointer and a size or an end, from a container and from a span of
		// dynamic extent are explicit when this span's extent is static (C++20's explicit(extent != dynamic_extent)).
		template <size_t E = Extent, enable_if_t<E == eastl::dynamic_extent, int> = 0>
		EA_CONSTEXPR span(pointer ptr, index_type count) EA_NOEXCEPT
			: mStorage(ptr, count) {}

		template <size_t E = Extent, enable_if_t<E != eastl::dynamic_extent, int> = 0>
		EA_CONSTEXPR explicit span(pointer ptr, index_type count) EA_NOEXCEPT
			: mStorage(ptr, count) {}

		template <size_t E = Extent, enable_if_t<E == eastl::dynamic_extent, int> = 0>
		EA_CONSTEXPR span(pointer pBegin, pointer pEnd) EA_NOEXCEPT
			: mStorage(pBegin, static_cast<index_type>(pEnd - pBegin)) {}

		template <size_t E = Extent, enable_if_t<E != eastl::dynamic_extent, int> = 0>
		EA_CONSTEXPR explicit span(pointer pBegin, pointer pEnd) EA_NOEXCEPT
			: mStorage(pBegin, static_cast<index_type>(pEnd - pBegin)) {}
		            ~span() EA_NOEXCEPT = default;

		// copy-assignment operator
		EA_CPP14_CONSTEXPR span& operator=(const span& other) EA_NOEXCEPT = default;

		// conversion constructors for c-array and eastl::array
		template <size_t N, typename = enable_if_t<(Extent == eastl::dynamic_extent || N == Extent)>> 
		EA_CONSTEXPR span(element_type (&arr)[N]) EA_NOEXCEPT;

		// Like std, from an array of any element type that converts (so span<int> doesn't take a const array).
		template <typename U, size_t N, typename = enable_if_t<(Extent == eastl::dynamic_extent || N == Extent) && is_convertible_v<U (*)[], element_type (*)[]>>>
		EA_CONSTEXPR span(eastl::array<U, N>& arr) EA_NOEXCEPT
			: span(arr.data(), static_cast<index_type>(N)) {}

		template <typename U, size_t N, typename = enable_if_t<(Extent == eastl::dynamic_extent || N == Extent) && is_convertible_v<const U (*)[], element_type (*)[]>>>
		EA_CONSTEXPR span(const eastl::array<U, N>& arr) EA_NOEXCEPT
			: span(arr.data(), static_cast<index_type>(N)) {}

		// SfinaeForGenericContainers
		//
		template <typename Container>
		using SfinaeForGenericContainers =
		    enable_if_t<!Internal::is_eastl_span<remove_cv_t<Container>>::value && !Internal::is_any_eastl_array<remove_cv_t<Container>>::value &&
		                !is_array_v<Container> &&
		                Internal::HasSizeAndData<Container>::value &&
		                is_convertible_v<remove_pointer_t<decltype(eastl::data(eastl::declval<Container&>()))> (*)[], element_type (*)[]>>;

		// generic container conversion constructors
		template <typename Container, typename = SfinaeForGenericContainers<Container>, size_t E = Extent, enable_if_t<E == eastl::dynamic_extent, int> = 0>
		EA_CONSTEXPR span(Container& cont)
			: span(static_cast<pointer>(eastl::data(cont)), static_cast<index_type>(eastl::size(cont))) {}

		template <typename Container, typename = SfinaeForGenericContainers<Container>, size_t E = Extent, enable_if_t<E != eastl::dynamic_extent, int> = 0>
		EA_CONSTEXPR explicit span(Container& cont)
			: span(static_cast<pointer>(eastl::data(cont)), static_cast<index_type>(eastl::size(cont))) {}

		template <typename Container, typename = SfinaeForGenericContainers<const Container>, size_t E = Extent, enable_if_t<E == eastl::dynamic_extent, int> = 0>
		EA_CONSTEXPR span(const Container& cont)
			: span(static_cast<pointer>(eastl::data(cont)), static_cast<index_type>(eastl::size(cont))) {}

		template <typename Container, typename = SfinaeForGenericContainers<const Container>, size_t E = Extent, enable_if_t<E != eastl::dynamic_extent, int> = 0>
		EA_CONSTEXPR explicit span(const Container& cont)
			: span(static_cast<pointer>(eastl::data(cont)), static_cast<index_type>(eastl::size(cont))) {}

		// From another span: explicit when this extent is static and the other's is dynamic, as in std.
		template <typename U, size_t N,
		          enable_if_t<(Extent == eastl::dynamic_extent || N == eastl::dynamic_extent || N == Extent) && is_convertible_v<U (*)[], element_type (*)[]> &&
		                      !(Extent != eastl::dynamic_extent && N == eastl::dynamic_extent), int> = 0>
		EA_CONSTEXPR span(const span<U, N>& s) EA_NOEXCEPT
			: span(s.data(), s.size()) {}

		template <typename U, size_t N,
		          enable_if_t<is_convertible_v<U (*)[], element_type (*)[]> && (Extent != eastl::dynamic_extent && N == eastl::dynamic_extent), int> = 0>
		EA_CONSTEXPR explicit span(const span<U, N>& s) EA_NOEXCEPT
			: span(s.data(), s.size()) {}

		// subviews
		template<size_t Count>
		EA_CPP14_CONSTEXPR span<element_type, Count> first() const EA_NOEXCEPT;
		EA_CPP14_CONSTEXPR span<element_type, dynamic_extent> first(size_t Count) const EA_NOEXCEPT;

		template<size_t Count>
		EA_CPP14_CONSTEXPR span<element_type, Count> last() const EA_NOEXCEPT;
		EA_CPP14_CONSTEXPR span<element_type, dynamic_extent> last(size_t Count) const EA_NOEXCEPT;

		template <size_t Offset, size_t Count = dynamic_extent>
		EA_CONSTEXPR span<element_type, Internal::SubspanExtent<Extent, Offset, Count>::value> subspan() const EA_NOEXCEPT;
		EA_CONSTEXPR span<element_type, dynamic_extent> subspan(size_t Offset, size_t Count = dynamic_extent) const EA_NOEXCEPT;

		// observers
		EA_CONSTEXPR pointer    data() const EA_NOEXCEPT;
		EA_CONSTEXPR index_type size() const EA_NOEXCEPT;
		EA_CONSTEXPR index_type size_bytes() const EA_NOEXCEPT;
		EA_CONSTEXPR bool       empty() const EA_NOEXCEPT;

		// subscript operators, element access
		EA_CONSTEXPR reference front() const EA_NOEXCEPT;
		EA_CONSTEXPR reference back() const EA_NOEXCEPT;
		EA_CONSTEXPR reference operator[](index_type idx) const EA_NOEXCEPT;
		EA_CONSTEXPR reference operator()(index_type idx) const EA_NOEXCEPT;

		// iterator support
		EA_CONSTEXPR iterator begin() const EA_NOEXCEPT;
		EA_CONSTEXPR iterator end() const EA_NOEXCEPT;
		EA_CONSTEXPR const_iterator cbegin() const EA_NOEXCEPT;
		EA_CONSTEXPR const_iterator cend() const EA_NOEXCEPT;
		EA_CONSTEXPR reverse_iterator rbegin() const EA_NOEXCEPT;
		EA_CONSTEXPR reverse_iterator rend() const EA_NOEXCEPT;
		EA_CONSTEXPR const_reverse_iterator crbegin() const EA_NOEXCEPT;
		EA_CONSTEXPR const_reverse_iterator crend() const EA_NOEXCEPT;

	private:
		Internal::SpanStorage<T, Extent> mStorage;

	private:
		EA_CONSTEXPR bool bounds_check(size_t) const EA_NOEXCEPT;  // utility used in asserts
	};


	///////////////////////////////////////////////////////////////////////////
	// template deduction guides 
	///////////////////////////////////////////////////////////////////////////
	#ifdef __cpp_deduction_guides
		template<class T, size_t N> span(T (&)[N]) ->           span <T, N>;
		template<class T, size_t N> span(array<T, N>&) ->       span <T, N>;
		template<class T, size_t N> span(const array<T, N>&) -> span <const T, N>;
		template<class Container>   span(Container&) ->         span <typename Container::value_type>;
		template<class Container>   span(const Container&) ->   span <const typename Container::value_type>;
	#endif


	///////////////////////////////////////////////////////////////////////////
	// comparison operators
	///////////////////////////////////////////////////////////////////////////

	template <class T, size_t X, class U, size_t Y>
	EA_CONSTEXPR bool operator==(span<T, X> l, span<U, Y> r)
	{
		return (l.size() == r.size()) && eastl::equal(l.begin(), l.end(), r.begin());
	}

	template <class T, size_t X, class U, size_t Y>
	EA_CONSTEXPR bool operator<(span<T, X> l, span<U, Y> r)
	{
		return eastl::lexicographical_compare(l.begin(), l.end(), r.begin(), r.end());
	}

	template <class T, size_t X, class U, size_t Y>
	EA_CONSTEXPR bool operator!=(span<T, X> l, span<U, Y> r) { return !(l == r); }

	template <class T, size_t X, class U, size_t Y>
	EA_CONSTEXPR bool operator<=(span<T, X> l, span<U, Y> r) { return !(r < l); }

	template <class T, size_t X, class U, size_t Y>
	EA_CONSTEXPR bool operator>(span<T, X> l, span<U, Y> r) { return r < l; }

	template <class T, size_t X, class U, size_t Y>
	EA_CONSTEXPR bool operator>=(span<T, X> l, span<U, Y> r) { return !(l < r); }


	///////////////////////////////////////////////////////////////////////////
	// as_bytes, as_writable_bytes
	//
	// A view of the span's elements as bytes (std::byte, as in std), of extent sizeof(T) * Extent when Extent is
	// static. as_writable_bytes is only for spans of non-const elements.
	///////////////////////////////////////////////////////////////////////////

	template <class T, size_t X>
	inline span<const std::byte, X == dynamic_extent ? dynamic_extent : sizeof(T) * X> as_bytes(span<T, X> s) EA_NOEXCEPT
	{
		return span<const std::byte, X == dynamic_extent ? dynamic_extent : sizeof(T) * X>(reinterpret_cast<const std::byte*>(s.data()), s.size_bytes());
	}

	template <class T, size_t X, typename = enable_if_t<!is_const_v<T>>>
	inline span<std::byte, X == dynamic_extent ? dynamic_extent : sizeof(T) * X> as_writable_bytes(span<T, X> s) EA_NOEXCEPT
	{
		return span<std::byte, X == dynamic_extent ? dynamic_extent : sizeof(T) * X>(reinterpret_cast<std::byte*>(s.data()), s.size_bytes());
	}


	///////////////////////////////////////////////////////////////////////////
	// ctor implementations
	///////////////////////////////////////////////////////////////////////////


	template <typename T, size_t Extent>
	template <size_t N, typename>
	EA_CONSTEXPR span<T, Extent>::span(element_type(&arr)[N]) EA_NOEXCEPT
		: span(arr, static_cast<index_type>(N))
	{
	}


	///////////////////////////////////////////////////////////////////////////
	// member function implementations
	///////////////////////////////////////////////////////////////////////////

	template <typename T, size_t Extent>
	EA_CONSTEXPR typename span<T, Extent>::pointer span<T, Extent>::data() const EA_NOEXCEPT
	{
		return mStorage.mpData;
	}

	template <typename T, size_t Extent>
	EA_CONSTEXPR typename span<T, Extent>::index_type span<T, Extent>::size() const EA_NOEXCEPT
	{
		return mStorage.mnSize;
	}

	template <typename T, size_t Extent>
	EA_CONSTEXPR typename span<T, Extent>::index_type span<T, Extent>::size_bytes() const EA_NOEXCEPT
	{
		return size() * sizeof(element_type);
	}

	template <typename T, size_t Extent>
	EA_CONSTEXPR bool span<T, Extent>::empty() const EA_NOEXCEPT
	{
		return size() == 0;
	}

	template <typename T, size_t Extent>
	EA_CONSTEXPR typename span<T, Extent>::reference span<T, Extent>::front() const EA_NOEXCEPT
	{
		EASTL_ASSERT_MSG(!empty(), "undefined behavior accessing an empty span");

		return mStorage.mpData[0];
	}

	template <typename T, size_t Extent>
	EA_CONSTEXPR typename span<T, Extent>::reference span<T, Extent>::back() const EA_NOEXCEPT
	{
		EASTL_ASSERT_MSG(!empty(), "undefined behavior accessing an empty span");

		return mStorage.mpData[mStorage.mnSize - 1];
	}

	template <typename T, size_t Extent>
	EA_CONSTEXPR typename span<T, Extent>::reference span<T, Extent>::operator[](index_type idx) const EA_NOEXCEPT
	{
		EASTL_ASSERT_MSG(!empty(),          "undefined behavior accessing an empty span");
		EASTL_ASSERT_MSG(bounds_check(idx), "undefined behavior accessing out of bounds");

		return mStorage.mpData[idx];
	}

	template <typename T, size_t Extent>
	EA_CONSTEXPR typename span<T, Extent>::reference span<T, Extent>::operator()(index_type idx) const EA_NOEXCEPT
	{
		EASTL_ASSERT_MSG(!empty(),          "undefined behavior accessing an empty span");
		EASTL_ASSERT_MSG(bounds_check(idx), "undefined behavior accessing out of bounds");

		return mStorage.mpData[idx];
	}

	template <typename T, size_t Extent>
	EA_CONSTEXPR typename span<T, Extent>::iterator span<T, Extent>::begin() const EA_NOEXCEPT
	{
		return mStorage.mpData;
	}

	template <typename T, size_t Extent>
	EA_CONSTEXPR typename span<T, Extent>::iterator span<T, Extent>::end() const EA_NOEXCEPT
	{
		return mStorage.mpData + mStorage.mnSize;
	}

	template <typename T, size_t Extent>
	EA_CONSTEXPR typename span<T, Extent>::const_iterator span<T, Extent>::cbegin() const EA_NOEXCEPT
	{
		return mStorage.mpData;
	}

	template <typename T, size_t Extent>
	EA_CONSTEXPR typename span<T, Extent>::const_iterator span<T, Extent>::cend() const EA_NOEXCEPT
	{
		return mStorage.mpData + mStorage.mnSize;
	}

	template <typename T, size_t Extent>
	EA_CONSTEXPR typename span<T, Extent>::reverse_iterator span<T, Extent>::rbegin() const EA_NOEXCEPT
	{
		return reverse_iterator(mStorage.mpData + mStorage.mnSize);
	}

	template <typename T, size_t Extent>
	EA_CONSTEXPR typename span<T, Extent>::reverse_iterator span<T, Extent>::rend() const EA_NOEXCEPT
	{
		return reverse_iterator(mStorage.mpData);
	}

	template <typename T, size_t Extent>
	EA_CONSTEXPR typename span<T, Extent>::const_reverse_iterator span<T, Extent>::crbegin() const EA_NOEXCEPT
	{
		return const_reverse_iterator(mStorage.mpData + mStorage.mnSize);
	}

	template <typename T, size_t Extent>
	EA_CONSTEXPR typename span<T, Extent>::const_reverse_iterator span<T, Extent>::crend() const EA_NOEXCEPT
	{
		return const_reverse_iterator(mStorage.mpData);
	}

	template <typename T, size_t Extent>
	template <size_t Count>
	EA_CPP14_CONSTEXPR span<typename span<T, Extent>::element_type, Count> span<T, Extent>::first() const EA_NOEXCEPT
	{
		EASTL_ASSERT_MSG(Count <= size(), "undefined behavior accessing out of bounds");
		return span<element_type, Count>(data(), static_cast<index_type>(Count));
	}

	template <typename T, size_t Extent>
	EA_CPP14_CONSTEXPR span<typename span<T, Extent>::element_type, dynamic_extent> 
	span<T, Extent>::first(size_t sz) const EA_NOEXCEPT
	{
		EASTL_ASSERT_MSG(sz <= size(), "undefined behavior accessing out of bounds");
		return {data(), static_cast<index_type>(sz)};
	}

	template <typename T, size_t Extent>
	template <size_t Count>
	EA_CPP14_CONSTEXPR span<typename span<T, Extent>::element_type, Count> span<T, Extent>::last() const EA_NOEXCEPT
	{
		EASTL_ASSERT_MSG(Count <= size(), "undefined behavior accessing out of bounds");
		return span<element_type, Count>(data() + size() - Count, static_cast<index_type>(Count));
	}

	template <typename T, size_t Extent>
	EA_CPP14_CONSTEXPR span<typename span<T, Extent>::element_type, dynamic_extent> 
	span<T, Extent>::last(size_t sz) const EA_NOEXCEPT
	{
		EASTL_ASSERT_MSG(sz <= size(), "undefined behavior accessing out of bounds");
		return {data() + size() - sz, static_cast<index_type>(sz)};
	}

	template <typename T, size_t Extent>
	template <size_t Offset, size_t Count>
	EA_CONSTEXPR span<typename span<T, Extent>::element_type, Internal::SubspanExtent<Extent, Offset, Count>::value>
	span<T, Extent>::subspan() const EA_NOEXCEPT
	{
		EASTL_ASSERT_MSG(Offset <= size(),                                       "undefined behaviour accessing out of bounds");
		EASTL_ASSERT_MSG(Count == dynamic_extent || Count <= (size() - Offset), "undefined behaviour exceeding size of span");

		return span<element_type, Internal::SubspanExtent<Extent, Offset, Count>::value>(data() + Offset, eastl_size_t(Count == dynamic_extent ? size() - Offset : Count));
	}

	template <typename T, size_t Extent>
	EA_CONSTEXPR span<typename span<T, Extent>::element_type, dynamic_extent>
	span<T, Extent>::subspan(size_t offset, size_t count) const EA_NOEXCEPT
	{
		EASTL_ASSERT_MSG(offset <= size(),                                      "undefined behaviour accessing out of bounds");
		EASTL_ASSERT_MSG(count == dynamic_extent || count <= (size() - offset), "undefined behaviour exceeding size of span");

		return {data() + offset, eastl_size_t(count == dynamic_extent ? size() - offset : count)};
	}

	template <typename T, size_t Extent>
	EA_CONSTEXPR bool span<T, Extent>::bounds_check(size_t offset) const EA_NOEXCEPT
	{
		return offset < size();
	}
}

#endif // EASTL_SPAN_H  
