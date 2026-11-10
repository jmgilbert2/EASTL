///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_ITERATOR_H
#define EASTL_ITERATOR_H


#include <EASTL/internal/config.h>
#include <EASTL/internal/move_help.h>
#include <EASTL/internal/type_detected.h>
#include <EASTL/internal/type_void_t.h>
#include <EASTL/internal/memory_base.h>
#include <EASTL/compare.h> // <compare> for the iterator adaptors' operator<=>, when three-way comparison is available.
#include <EASTL/initializer_list.h>

#include <stddef.h>

// If the user has specified that we use std iterator
// categories instead of EASTL iterator categories,
// then #include <iterator>.
#if EASTL_STD_ITERATOR_CATEGORY_ENABLED
	#include <iterator>
#endif


EA_DISABLE_VC_WARNING(4619); // There is no warning number 'number'.
EA_DISABLE_VC_WARNING(4217); // Member template functions cannot be used for copy-assignment or copy-construction.

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif



namespace eastl
{
	/// iterator_status_flag
	///
	/// Defines the validity status of an iterator. This is primarily used for
	/// iterator validation in debug builds. These are implemented as OR-able
	/// flags (as opposed to mutually exclusive values) in order to deal with
	/// the nature of iterator status. In particular, an iterator may be valid
	/// but not dereferencable, as in the case with an iterator to container end().
	/// An iterator may be valid but also dereferencable, as in the case with an
	/// iterator to container begin().
	///
	enum iterator_status_flag
	{
		isf_none            = 0x00, /// This is called none and not called invalid because it is not strictly the opposite of invalid.
		isf_valid           = 0x01, /// The iterator is valid, which means it is in the range of [begin, end].
		isf_current         = 0x02, /// The iterator is valid and points to the same element it did when created. For example, if an iterator points to vector::begin() but an element is inserted at the front, the iterator is valid but not current. Modification of elements in place do not make iterators non-current.
		isf_can_dereference = 0x04  /// The iterator is dereferencable, which means it is in the range of [begin, end). It may or may not be current.
	};



	// The following declarations are taken directly from the C++ standard document.
	//    input_iterator_tag, etc.
	//    iterator
	//    iterator_traits
	//    reverse_iterator

	// Iterator categories
	// Every iterator is defined as belonging to one of the iterator categories that
	// we define here. These categories come directly from the C++ standard.
	#if !EASTL_STD_ITERATOR_CATEGORY_ENABLED // If we are to use our own iterator category definitions...
		struct input_iterator_tag { };
		struct output_iterator_tag { };
		struct forward_iterator_tag       : public input_iterator_tag { };
		struct bidirectional_iterator_tag : public forward_iterator_tag { };
		struct random_access_iterator_tag : public bidirectional_iterator_tag { };
		// Originally an extension to the C++ standard, standardized in C++20.
		// Contiguous ranges are more than random access, they are physically contiguous.
		// Note: Pointers are contiguous but the specialization of iterator_traits for pointers defines
		// iterator_traits<T>::iterator_category as random_access_iterator_tag and thus users must
		// explicitly check both the iterator_category and the type.
		struct contiguous_iterator_tag    : public random_access_iterator_tag { };
	#else
		using input_iterator_tag = std::input_iterator_tag;
		using output_iterator_tag = std::output_iterator_tag;
		using forward_iterator_tag = std::forward_iterator_tag;
		using bidirectional_iterator_tag = std::bidirectional_iterator_tag;
		using random_access_iterator_tag = std::random_access_iterator_tag;
	#if defined(EA_COMPILER_CPP20_ENABLED)
		using contiguous_iterator_tag = std::contiguous_iterator_tag;
	#endif
	#endif


	// struct iterator_traits
	namespace internal
	{
		// Helper to make iterator_traits SFINAE friendly as N3844 requires. As in C++20, an iterator with the other
		// four member types but no pointer member has iterator traits, with pointer = void.
		template <typename Iterator, class = void>
		struct iterator_traits_pointer { typedef void type; };

		template <typename Iterator>
		struct iterator_traits_pointer<Iterator, void_t<typename Iterator::pointer>> { typedef typename Iterator::pointer type; };

		template <typename Iterator, class = void>
		struct default_iterator_traits {};

		template <typename Iterator>
		struct default_iterator_traits<
			Iterator,
			void_t<
				typename Iterator::iterator_category,
				typename Iterator::value_type,
				typename Iterator::difference_type,
				typename Iterator::reference
			>
		>
		{
			typedef typename Iterator::iterator_category                iterator_category;
			typedef typename Iterator::value_type                       value_type;
			typedef typename Iterator::difference_type                  difference_type;
			typedef typename iterator_traits_pointer<Iterator>::type    pointer;
			typedef typename Iterator::reference                        reference;
		};

		// As in C++20, only pointers to object types have iterator traits (iterator_traits<void*> has no members),
		// and the value type is cv-unqualified.
		template <typename T, bool = eastl::is_object<T>::value>
		struct pointer_iterator_traits {};

		template <typename T>
		struct pointer_iterator_traits<T, true>
		{
			typedef eastl::random_access_iterator_tag iterator_category;
		#if !EASTL_STD_ITERATOR_CATEGORY_ENABLED || defined(EA_COMPILER_CPP20_ENABLED) // Where contiguous_iterator_tag exists.
			typedef eastl::contiguous_iterator_tag    iterator_concept;
		#endif
			typedef typename eastl::remove_cv<T>::type value_type;
			typedef ptrdiff_t                         difference_type;
			typedef T*                                pointer;
			typedef T&                                reference;
		};

		template <typename T>
		using arrow_operator_t = decltype(eastl::declval<T>().operator->());

		// bool, when the comparison Result converts to bool: the constraint of the iterator adaptors' comparison
		// operators, as in C++20, so that they don't participate for base iterators that can't be compared.
		template <typename Result>
		using iterator_comparison_t = eastl::enable_if_t<eastl::is_convertible<Result, bool>::value, bool>;
	}

	template <typename Iterator>
	struct iterator_traits : internal::default_iterator_traits<Iterator> {};

	template <typename T>
	struct iterator_traits<T*> : internal::pointer_iterator_traits<T> {};


	/// iter_move
	///
	/// Returns the element an iterator refers to as an rvalue, for moving it: eastl::move(*it) when *it is an lvalue,
	/// else *it itself (an iterator that returns a proxy by value). Like swap, algorithms call it unqualified, so that
	/// an iterator can provide its own iter_move, found by argument-dependent lookup, when moving through its proxy
	/// takes something else: tuple_vector's iterator returns a tuple<Ts&&...>. This is the iter_move customization
	/// point that C++20's std::ranges::iter_move uses, as a plain function like eastl::swap. It takes the iterator by
	/// value, as the algorithms pass iterators: algorithms call it with expressions like last - 1, and when it bound them
	/// to a const reference, MSVC no longer inlined adjust_heap into sort_heap (heap_sort of ints was 2-4% slower).
	///
	/// Example usage:
	///     using eastl::iter_move; // Only needed outside namespace eastl.
	///     value_type temp(iter_move(it));
	///
	namespace internal
	{
		template <typename Reference>
		using iter_move_result_t = conditional_t<is_lvalue_reference<Reference>::value, remove_reference_t<Reference>&&, Reference>;
	}

	template <typename Iterator>
	EA_CPP14_CONSTEXPR internal::iter_move_result_t<decltype(*eastl::declval<const Iterator&>())> iter_move(Iterator it) EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(*it))
	{
		return static_cast<internal::iter_move_result_t<decltype(*it)>>(*it);
	}




	/// is_iterator_wrapper
	///
	/// Tells if an Iterator type is a wrapper type as opposed to a regular type.
	/// Relies on the class declaring a member function called unwrap.
	///
	/// Examples of wrapping iterators:
	///     generic_iterator
	///     move_iterator
	///     reverse_iterator<T> (if T is a wrapped iterator)
	/// Examples of non-wrapping iterators:
	///     iterator
	///     list::iterator
	///     char*
	///
	/// Example behavior:
	///     is_iterator_wrapper(int*)::value												=> false
	///     is_iterator_wrapper(eastl::array<char>*)::value									=> false
	///     is_iterator_wrapper(eastl::vector<int>::iterator)::value						=> false
	///     is_iterator_wrapper(eastl::generic_iterator<int*>)::value						=> true
	///     is_iterator_wrapper(eastl::move_iterator<eastl::array<int>::iterator>)::value	=> true
	///     is_iterator_wrapper(eastl::reverse_iterator<int*>)::value						=> false
	///     is_iterator_wrapper(eastl::reverse_iterator<eastl::move_iterator<int*>>)::value	=> true
	///
	template<typename Iterator>
	class EASTL_REMOVE_AT_2024_SEPT is_iterator_wrapper
	{
#if defined(EA_COMPILER_CLANG) || defined(EA_COMPILER_CLANG_CL)
		// Using a default template type parameter trick here because
		// of a bug in clang that makes the other implementation not
		// work when unwrap() is private and this is class is a
		// friend.
		// See: https://bugs.llvm.org/show_bug.cgi?id=25334
		template<typename T, typename U = decltype(eastl::declval<T>().unwrap())>
		using detect_has_unwrap = U;
#else
		// Note: the above implementation does not work on GCC when
		// unwrap() is private and this class is a friend. So we're
		// forced to diverge here to support both GCC and clang.
		template<typename T>
		using detect_has_unwrap = decltype(eastl::declval<T>().unwrap());
#endif
	public:
		static const bool value = eastl::is_detected<detect_has_unwrap, Iterator>::value;
	};


	/// unwrap_iterator
	///
	/// Takes a wrapper Iterator (e.g. move_iterator, reverse_iterator, generic_iterator) instance
	/// and returns the wrapped iterator type. If Iterator is not a wrapper (including being a pointer),
	/// or is not an iterator, then this function returns it as-is.
	/// unwrap_iterator unwraps only a single layer of iterator at a time. You need to call it twice,
	/// for example, to unwrap two layers of iterators.
	///
	/// Example usage:
	///     int* pInt             = unwrap_iterator(&pIntArray[15]);
	///     int* pInt             = unwrap_iterator(generic_iterator(&pIntArray[15]));
	///     MyVector::iterator it = unwrap_iterator(myVector.begin());
	///     MyVector::iterator it = unwrap_iterator(move_iterator(myVector.begin()));
	///
	template <typename Iterator, bool isWrapper>
	struct EASTL_REMOVE_AT_2024_SEPT is_iterator_wrapper_helper
	{
		using iterator_type = Iterator;

		static iterator_type get_unwrapped(Iterator it) { return it; }
	};


	// Note: some compilers (notably GCC) trigger deprecation warnings when doing template
	// specialization if the main template is derpecated, so turn the warning off here. If this
	// specialization is used, the warning will still trigger in the user code, this just
	// disables the warning in this declaration.
EASTL_INTERNAL_DISABLE_DEPRECATED()
	template <typename Iterator>
	struct EASTL_REMOVE_AT_2024_SEPT is_iterator_wrapper_helper<Iterator, true>
	{
		// get_unwrapped must return by value since we're returning
		// it.unwrap(), and `it` will be out of scope as soon as
		// get_unwrapped returns.
		using iterator_type =
		    typename eastl::remove_cvref<decltype(eastl::declval<Iterator>().unwrap())>::type;

		static iterator_type get_unwrapped(Iterator it) { return it.unwrap(); }
	};


	template <typename Iterator>
	EASTL_REMOVE_AT_2024_SEPT inline typename is_iterator_wrapper_helper<Iterator, eastl::is_iterator_wrapper<Iterator>::value>::iterator_type unwrap_iterator(Iterator it)
		{ return eastl::is_iterator_wrapper_helper<Iterator, eastl::is_iterator_wrapper<Iterator>::value>::get_unwrapped(it); }
EASTL_INTERNAL_RESTORE_DEPRECATED()



	/// reverse_iterator
	///
	/// From the C++ standard:
	/// Bidirectional and random access iterators have corresponding reverse
	/// iterator adaptors that iterate through the data structure in the
	/// opposite direction. They have the same signatures as the corresponding
	/// iterators. The fundamental relation between a reverse iterator and its
	/// corresponding iterator i is established by the identity:
	///     &*(reverse_iterator(i)) == &*(i - 1).
	/// This mapping is dictated by the fact that while there is always a pointer
	/// past the end of an array, there might not be a valid pointer before the
	/// beginning of an array.
	///
	template <typename Iterator>
	class reverse_iterator
	{
	public:
		typedef Iterator														iterator_type;
		typedef typename eastl::iterator_traits<Iterator>::iterator_category	iterator_category;
		typedef typename eastl::iterator_traits<Iterator>::value_type			value_type;
		typedef typename eastl::iterator_traits<Iterator>::difference_type		difference_type;
		typedef typename eastl::iterator_traits<Iterator>::pointer				pointer;
		typedef typename eastl::iterator_traits<Iterator>::reference			reference;
		// As in C++20: random access for a random access iterator, else bidirectional.
		typedef typename eastl::conditional<eastl::is_base_of<eastl::random_access_iterator_tag, iterator_category>::value,
		                                    eastl::random_access_iterator_tag, eastl::bidirectional_iterator_tag>::type iterator_concept;

	protected:
		Iterator mIterator;

	public:
		EA_CPP14_CONSTEXPR reverse_iterator() EA_NOEXCEPT_IF(eastl::is_nothrow_default_constructible<Iterator>::value) // It's important that we construct mIterator, because if Iterator
			: mIterator() { }					   // is a pointer, there's a difference between doing it and not.

		EA_CPP14_CONSTEXPR explicit reverse_iterator(iterator_type i) EA_NOEXCEPT_IF(eastl::is_nothrow_move_constructible<Iterator>::value)
			: mIterator(i) { }

		// Like std, the converting constructor and assignment participate only if the base iterator converts.
		template <typename U, typename = eastl::enable_if_t<!eastl::is_same<U, Iterator>::value && eastl::is_convertible<const U&, Iterator>::value>>
		EA_CPP14_CONSTEXPR reverse_iterator(const reverse_iterator<U>& ri) EA_NOEXCEPT_IF((eastl::is_nothrow_constructible<Iterator, const U&>::value))
			: mIterator(ri.base()) { }

		template <typename U, typename = eastl::enable_if_t<!eastl::is_same<U, Iterator>::value && eastl::is_convertible<const U&, Iterator>::value &&
		                                                    eastl::is_assignable<Iterator&, const U&>::value>>
		EA_CPP14_CONSTEXPR reverse_iterator<Iterator>& operator=(const reverse_iterator<U>& ri) EA_NOEXCEPT_IF((eastl::is_nothrow_assignable<Iterator&, const U&>::value))
			{ mIterator = ri.base(); return *this; }

		EA_CPP14_CONSTEXPR iterator_type base() const EA_NOEXCEPT_IF(eastl::is_nothrow_copy_constructible<Iterator>::value)
			{ return mIterator; }

		EA_CPP14_CONSTEXPR reference operator*() const EA_NOEXCEPT_IF(eastl::is_nothrow_copy_constructible<Iterator>::value && EA_NOEXCEPT_EXPR(*--eastl::declval<Iterator&>()))
		{
			iterator_type i(mIterator);
			return *--i;
		}

		// Like std, only for a pointer or a base iterator with operator->, which is used.
		template <typename I = Iterator, eastl::enable_if_t<eastl::is_pointer<I>::value, int> = 0>
		EA_CPP14_CONSTEXPR pointer operator->() const EA_NOEXCEPT
		{
			iterator_type i(mIterator);
			return --i;
		}

		template <typename I = Iterator, eastl::enable_if_t<!eastl::is_pointer<I>::value && eastl::is_detected<internal::arrow_operator_t, const I&>::value, int> = 0>
		EA_CPP14_CONSTEXPR pointer operator->() const
			EA_NOEXCEPT_IF(eastl::is_nothrow_copy_constructible<I>::value && EA_NOEXCEPT_EXPR(--eastl::declval<I&>()) && EA_NOEXCEPT_EXPR(eastl::declval<I&>().operator->()))
		{
			iterator_type i(mIterator);
			--i;
			return i.operator->();
		}

		// Moves through the base iterator's iter_move, like std::reverse_iterator.
		friend EA_CPP14_CONSTEXPR decltype(auto) iter_move(const reverse_iterator& ri)
			EA_NOEXCEPT_IF(eastl::is_nothrow_copy_constructible<Iterator>::value && EA_NOEXCEPT_EXPR(iter_move(--eastl::declval<Iterator&>())))
		{
			iterator_type i(ri.mIterator);
			return iter_move(--i);
		}

		EA_CPP14_CONSTEXPR reverse_iterator& operator++() EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(--mIterator))
			{ --mIterator; return *this; }

		EA_CPP14_CONSTEXPR reverse_iterator operator++(int) EA_NOEXCEPT_IF(eastl::is_nothrow_copy_constructible<Iterator>::value && EA_NOEXCEPT_EXPR(--mIterator))
		{
			reverse_iterator ri(*this);
			--mIterator;
			return ri;
		}

		EA_CPP14_CONSTEXPR reverse_iterator& operator--() EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(++mIterator))
			{ ++mIterator; return *this; }

		EA_CPP14_CONSTEXPR reverse_iterator operator--(int) EA_NOEXCEPT_IF(eastl::is_nothrow_copy_constructible<Iterator>::value && EA_NOEXCEPT_EXPR(++mIterator))
		{
			reverse_iterator ri(*this);
			++mIterator;
			return ri;
		}

		EA_CPP14_CONSTEXPR reverse_iterator operator+(difference_type n) const EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(reverse_iterator(mIterator - n)))
			{ return reverse_iterator(mIterator - n); }

		EA_CPP14_CONSTEXPR reverse_iterator& operator+=(difference_type n) EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(mIterator -= n))
			{ mIterator -= n; return *this; }

		EA_CPP14_CONSTEXPR reverse_iterator operator-(difference_type n) const EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(reverse_iterator(mIterator + n)))
			{ return reverse_iterator(mIterator + n); }

		EA_CPP14_CONSTEXPR reverse_iterator& operator-=(difference_type n) EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(mIterator += n))
			{ mIterator += n; return *this; }

		// http://cplusplus.github.io/LWG/lwg-defects.html#386,
		// http://llvm.org/bugs/show_bug.cgi?id=17883
		// random_access_iterator operator[] is merely required to return something convertible to reference.
		// reverse_iterator operator[] can't necessarily know what to return as the underlying iterator
		// operator[] may return something other than reference.
		EA_CPP14_CONSTEXPR reference operator[](difference_type n) const EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(mIterator[-n - 1]))
			{ return mIterator[-n - 1]; }

		
    EASTL_INTERNAL_DISABLE_DEPRECATED() // 'is_iterator_wrapper': was declared deprecated
	private:
		using base_wrapped_iterator_type =
		    typename eastl::is_iterator_wrapper_helper<Iterator,
		                                               eastl::is_iterator_wrapper<Iterator>::value>::iterator_type;

		// Unwrapping interface, not part of the public API.
		template <typename U = iterator_type>
		EASTL_REMOVE_AT_2024_SEPT EA_CPP14_CONSTEXPR typename eastl::enable_if<eastl::is_iterator_wrapper<U>::value, reverse_iterator<base_wrapped_iterator_type>>::type unwrap() const
		{ return reverse_iterator<base_wrapped_iterator_type>(unwrap_iterator(mIterator)); }

		// The unwrapper helpers need access to unwrap() (when it exists).
		using this_type = reverse_iterator<Iterator>;
		friend is_iterator_wrapper_helper<this_type, is_iterator_wrapper<iterator_type>::value>;
		friend is_iterator_wrapper<this_type>;
	EASTL_INTERNAL_RESTORE_DEPRECATED()
	};


	// The C++ library working group has tentatively approved the usage of two
	// template parameters (Iterator1 and Iterator2) in order to allow reverse_iterators
	// and const_reverse iterators to be comparable. This is a similar issue to the
	// C++ defect report #179 regarding comparison of container iterators and const_iterators.
	//
	// libstdc++ reports that std::relops breaks the usage of two iterator types and if we
	// want to support relops then we need to also make versions of each of below with
	// a single template parameter to placate std::relops. But relops is hardly used due to
	// the troubles it causes and so we are avoiding support here until somebody complains about it.
	// Like std, each comparison participates only if the comparison of the base iterators it makes does.
	template <typename Iterator1, typename Iterator2>
	EA_CPP14_CONSTEXPR inline internal::iterator_comparison_t<decltype(eastl::declval<const Iterator1&>() == eastl::declval<const Iterator2&>())>
	operator==(const reverse_iterator<Iterator1>& a, const reverse_iterator<Iterator2>& b)
		EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(eastl::declval<const Iterator1&>() == eastl::declval<const Iterator2&>()))
		{ return a.base() == b.base(); }


	template <typename Iterator1, typename Iterator2>
	EA_CPP14_CONSTEXPR inline internal::iterator_comparison_t<decltype(eastl::declval<const Iterator1&>() > eastl::declval<const Iterator2&>())>
	operator<(const reverse_iterator<Iterator1>& a, const reverse_iterator<Iterator2>& b)
		EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(eastl::declval<const Iterator1&>() > eastl::declval<const Iterator2&>()))
		{ return a.base() > b.base(); }


	template <typename Iterator1, typename Iterator2>
	EA_CPP14_CONSTEXPR inline internal::iterator_comparison_t<decltype(eastl::declval<const Iterator1&>() != eastl::declval<const Iterator2&>())>
	operator!=(const reverse_iterator<Iterator1>& a, const reverse_iterator<Iterator2>& b)
		EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(eastl::declval<const Iterator1&>() != eastl::declval<const Iterator2&>()))
		{ return a.base() != b.base(); }


	template <typename Iterator1, typename Iterator2>
	EA_CPP14_CONSTEXPR inline internal::iterator_comparison_t<decltype(eastl::declval<const Iterator1&>() < eastl::declval<const Iterator2&>())>
	operator>(const reverse_iterator<Iterator1>& a, const reverse_iterator<Iterator2>& b)
		EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(eastl::declval<const Iterator1&>() < eastl::declval<const Iterator2&>()))
		{ return a.base() < b.base(); }


	template <typename Iterator1, typename Iterator2>
	EA_CPP14_CONSTEXPR inline internal::iterator_comparison_t<decltype(eastl::declval<const Iterator1&>() >= eastl::declval<const Iterator2&>())>
	operator<=(const reverse_iterator<Iterator1>& a, const reverse_iterator<Iterator2>& b)
		EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(eastl::declval<const Iterator1&>() >= eastl::declval<const Iterator2&>()))
		{ return a.base() >= b.base(); }


	template <typename Iterator1, typename Iterator2>
	EA_CPP14_CONSTEXPR inline internal::iterator_comparison_t<decltype(eastl::declval<const Iterator1&>() <= eastl::declval<const Iterator2&>())>
	operator>=(const reverse_iterator<Iterator1>& a, const reverse_iterator<Iterator2>& b)
		EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(eastl::declval<const Iterator1&>() <= eastl::declval<const Iterator2&>()))
		{ return a.base() <= b.base(); }


	#if defined(EA_COMPILER_HAS_THREE_WAY_COMPARISON)
	template <typename Iterator1, std::three_way_comparable_with<Iterator1> Iterator2>
	EA_CONSTEXPR inline std::compare_three_way_result_t<Iterator1, Iterator2>
	operator<=>(const reverse_iterator<Iterator1>& a, const reverse_iterator<Iterator2>& b)
		EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(eastl::declval<const Iterator2&>() <=> eastl::declval<const Iterator1&>()))
		{ return b.base() <=> a.base(); }
	#endif


	// Like std, the difference type is the base iterators' difference (b.base() - a.base()), and the operator
	// participates only if that difference is well-formed.
	template <typename Iterator1, typename Iterator2>
	EA_CPP14_CONSTEXPR inline auto
	operator-(const reverse_iterator<Iterator1>& a, const reverse_iterator<Iterator2>& b)
		EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(eastl::declval<const Iterator2&>() - eastl::declval<const Iterator1&>())) -> decltype(b.base() - a.base())
		{ return b.base() - a.base(); }


	template <typename Iterator>
	EA_CPP14_CONSTEXPR inline reverse_iterator<Iterator>
	operator+(typename reverse_iterator<Iterator>::difference_type n, const reverse_iterator<Iterator>& a)
		EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(a + n))
		{ return reverse_iterator<Iterator>(a.base() - n); }


	/// is_reverse_iterator
	///
	/// This is a type traits extension utility.
	/// Given an iterator, tells if it's a reverse_iterator vs anything else.
	/// If it's a reverse iterator wrapped by another iterator then value is false.
	/// To consider: Detect that if it's a move_iterator<reverse_iterator> and unwrap
	/// move_iterator so we can detect that underneath it's reverse_iterator.
	///
	template <typename T>
	struct EASTL_REMOVE_AT_2024_SEPT is_reverse_iterator
		: public eastl::false_type {};

	// Note: some compilers (notably GCC) trigger deprecation warnings when doing template
	// specialization if the main template is derpecated, so turn the warning off here. If this
	// specialization is used, the warning will still trigger in the user code, this just
	// disables the warning in this declaration.
EASTL_INTERNAL_DISABLE_DEPRECATED()
	template<typename Iterator>
	struct EASTL_REMOVE_AT_2024_SEPT is_reverse_iterator<eastl::reverse_iterator<Iterator>>
		: public eastl::true_type {};
EASTL_INTERNAL_RESTORE_DEPRECATED()

	/// unwrap_reverse_iterator is not implemented since there's no
	/// good use case and there's some abiguitiy. Note that
	/// unwrap_iterator(reverse_iterator<T>) returns
	/// reverse_iterator<unwrap(T)>. However, given what
	/// unwrap_generic_iterator and unwrap_move_iterator do, one might
	/// expect unwrap_reverse_iterator(reverse_iterator<T>) to return
	/// T, which is not the same. To avoid that confusion, and because
	/// there's no current use case for this, we don't provide
	/// unwrap_reverse_iterator.



	/// move_iterator
	///
	/// From the C++11 Standard, section 24.5.3.1:
	/// Class template move_iterator is an iterator adaptor with the same behavior as the underlying iterator
	/// except that its dereference operator implicitly converts the value returned by the underlying iterator's
	/// dereference operator to an rvalue reference. Some generic algorithms can be called with move iterators to
	/// replace copying with moving.

	template<typename Iterator>
	class move_iterator
	{
	public:
		typedef Iterator                                iterator_type;
		typedef iterator_traits<Iterator>               traits_type;
		typedef typename traits_type::iterator_category iterator_category; // todo: use is_contiguous_iterator<Iterator> to correctly identify pointers as contiguous?
		typedef typename traits_type::value_type        value_type;
		typedef typename traits_type::difference_type   difference_type;
		typedef Iterator                                pointer;
		// Dereferences through iter_move, like std::move_iterator since C++20.
		using reference = decltype(iter_move(eastl::declval<const Iterator&>()));
		// As in C++20: the strongest of random access, bidirectional, forward and input that the base iterator has.
		typedef typename eastl::conditional<eastl::is_base_of<eastl::random_access_iterator_tag, iterator_category>::value, eastl::random_access_iterator_tag,
		        typename eastl::conditional<eastl::is_base_of<eastl::bidirectional_iterator_tag, iterator_category>::value, eastl::bidirectional_iterator_tag,
		        typename eastl::conditional<eastl::is_base_of<eastl::forward_iterator_tag, iterator_category>::value, eastl::forward_iterator_tag,
		                                    eastl::input_iterator_tag>::type>::type>::type iterator_concept;

	protected:
		iterator_type mIterator;

	public:
		move_iterator() EA_NOEXCEPT_IF(eastl::is_nothrow_default_constructible<Iterator>::value)
		  : mIterator()
		{
		}

		explicit move_iterator(iterator_type mi) EA_NOEXCEPT_IF(eastl::is_nothrow_move_constructible<Iterator>::value)
		  : mIterator(mi) { }

		// Like std, the converting constructor and assignment participate only if the base iterator converts.
		template <typename U, typename = eastl::enable_if_t<!eastl::is_same<U, Iterator>::value && eastl::is_convertible<const U&, Iterator>::value>>
		move_iterator(const move_iterator<U>& mi) EA_NOEXCEPT_IF((eastl::is_nothrow_constructible<Iterator, const U&>::value))
		  : mIterator(mi.base())
		{
		}

		template <typename U, typename = eastl::enable_if_t<!eastl::is_same<U, Iterator>::value && eastl::is_convertible<const U&, Iterator>::value &&
		                                                    eastl::is_assignable<Iterator&, const U&>::value>>
		move_iterator& operator=(const move_iterator<U>& mi) EA_NOEXCEPT_IF((eastl::is_nothrow_assignable<Iterator&, const U&>::value))
		{
			mIterator = mi.base();
			return *this;
		}

		// Unconditionally noexcept: C++20's std::move_iterator::base() returns a const reference, noexcept.
		iterator_type base() const EA_NOEXCEPT
			{ return mIterator; }

		// iter_move takes the iterator by value: a throwing copy doesn't make the dereference potentially throwing, as std's.
		reference operator*() const EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(iter_move(eastl::declval<const Iterator&>())) || EA_NOEXCEPT_EXPR(*eastl::declval<const Iterator&>())) { return iter_move(mIterator); }

		pointer operator->() const EA_NOEXCEPT_IF(eastl::is_nothrow_copy_constructible<Iterator>::value)
			{ return mIterator; }

		move_iterator& operator++() EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(++mIterator))
		{
			++mIterator;
			return *this;
		}

		move_iterator operator++(int) EA_NOEXCEPT_IF(eastl::is_nothrow_copy_constructible<Iterator>::value && EA_NOEXCEPT_EXPR(++mIterator))
		{
			move_iterator tempMoveIterator = *this;
			++mIterator;
			return tempMoveIterator;
		}

		move_iterator& operator--() EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(--mIterator))
		{
			--mIterator;
			return *this;
		}

		move_iterator operator--(int) EA_NOEXCEPT_IF(eastl::is_nothrow_copy_constructible<Iterator>::value && EA_NOEXCEPT_EXPR(--mIterator))
		{
			move_iterator tempMoveIterator = *this;
			--mIterator;
			return tempMoveIterator;
		}

		move_iterator operator+(difference_type n) const EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(move_iterator(mIterator + n)))
			{ return move_iterator(mIterator + n); }

		move_iterator& operator+=(difference_type n) EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(mIterator += n))
		{
			mIterator += n;
			return *this;
		}

		move_iterator operator-(difference_type n) const EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(move_iterator(mIterator - n)))
			{ return move_iterator(mIterator - n); }

		move_iterator& operator-=(difference_type n) EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(mIterator -= n))
		{
			mIterator -= n;
			return *this;
		}

		reference operator[](difference_type n) const EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(iter_move(eastl::declval<const Iterator&>() + n)))
			{ return iter_move(mIterator + n); }

	EASTL_INTERNAL_DISABLE_DEPRECATED() // 'is_iterator_wrapper': was declared deprecated
	private:
		// Unwrapping interface, not part of the public API.
		EASTL_REMOVE_AT_2024_SEPT iterator_type unwrap() const
			{ return mIterator; }

		// The unwrapper helpers need access to unwrap().
		using this_type = move_iterator<Iterator>;
		friend is_iterator_wrapper_helper<this_type, true>;
		friend is_iterator_wrapper<this_type>;
	EASTL_INTERNAL_RESTORE_DEPRECATED()
	};

	// Like std, each comparison participates only if the comparison of the base iterators it makes does.
	template<typename Iterator1, typename Iterator2>
	inline internal::iterator_comparison_t<decltype(eastl::declval<const Iterator1&>() == eastl::declval<const Iterator2&>())>
	operator==(const move_iterator<Iterator1>& a, const move_iterator<Iterator2>& b)
		EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(eastl::declval<const Iterator1&>() == eastl::declval<const Iterator2&>()))
		{ return a.base() == b.base(); }


	template<typename Iterator1, typename Iterator2>
	inline internal::iterator_comparison_t<decltype(eastl::declval<const Iterator1&>() == eastl::declval<const Iterator2&>())>
	operator!=(const move_iterator<Iterator1>& a, const move_iterator<Iterator2>& b)
		EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(a == b))
		{ return !(a == b); }


	template<typename Iterator1, typename Iterator2>
	inline internal::iterator_comparison_t<decltype(eastl::declval<const Iterator1&>() < eastl::declval<const Iterator2&>())>
	operator<(const move_iterator<Iterator1>& a, const move_iterator<Iterator2>& b)
		EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(eastl::declval<const Iterator1&>() < eastl::declval<const Iterator2&>()))
		{ return a.base() < b.base(); }


	template<typename Iterator1, typename Iterator2>
	inline internal::iterator_comparison_t<decltype(eastl::declval<const Iterator2&>() < eastl::declval<const Iterator1&>())>
	operator<=(const move_iterator<Iterator1>& a, const move_iterator<Iterator2>& b)
		EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(b < a))
		{ return !(b < a); }


	template<typename Iterator1, typename Iterator2>
	inline internal::iterator_comparison_t<decltype(eastl::declval<const Iterator2&>() < eastl::declval<const Iterator1&>())>
	operator>(const move_iterator<Iterator1>& a, const move_iterator<Iterator2>& b)
		EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(b < a))
		{ return b < a; }


	template<typename Iterator1, typename Iterator2>
	inline internal::iterator_comparison_t<decltype(eastl::declval<const Iterator1&>() < eastl::declval<const Iterator2&>())>
	operator>=(const move_iterator<Iterator1>& a, const move_iterator<Iterator2>& b)
		EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(a < b))
		{ return !(a < b); }


	#if defined(EA_COMPILER_HAS_THREE_WAY_COMPARISON)
	template <typename Iterator1, std::three_way_comparable_with<Iterator1> Iterator2>
	inline std::compare_three_way_result_t<Iterator1, Iterator2>
	operator<=>(const move_iterator<Iterator1>& a, const move_iterator<Iterator2>& b)
		EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(eastl::declval<const Iterator1&>() <=> eastl::declval<const Iterator2&>()))
		{ return a.base() <=> b.base(); }
	#endif


	template<typename Iterator1, typename Iterator2>
	inline auto
	operator-(const move_iterator<Iterator1>& a, const move_iterator<Iterator2>& b)
		EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(eastl::declval<const Iterator1&>() - eastl::declval<const Iterator2&>())) -> decltype(a.base() - b.base())
		{ return a.base() - b.base(); }


	/// move_sentinel
	///
	/// The sentinel adaptor that goes with move_iterator, as in C++20: a move_iterator<I> compares equal to a
	/// move_sentinel<S> when its base iterator compares equal to the base sentinel, and their difference is the
	/// difference of the bases.
	///
	template <typename Sentinel>
	class move_sentinel
	{
	public:
		EA_CONSTEXPR move_sentinel() EA_NOEXCEPT_IF(eastl::is_nothrow_default_constructible<Sentinel>::value)
		  : mSentinel() {}

		EA_CONSTEXPR explicit move_sentinel(Sentinel s) EA_NOEXCEPT_IF(eastl::is_nothrow_move_constructible<Sentinel>::value)
		  : mSentinel(eastl::move(s)) {}

		template <typename S2, typename = eastl::enable_if_t<eastl::is_convertible<const S2&, Sentinel>::value>>
		EA_CONSTEXPR move_sentinel(const move_sentinel<S2>& s) EA_NOEXCEPT_IF((eastl::is_nothrow_constructible<Sentinel, const S2&>::value))
		  : mSentinel(s.base()) {}

		template <typename S2, typename = eastl::enable_if_t<eastl::is_assignable<Sentinel&, const S2&>::value>>
		EA_CPP14_CONSTEXPR move_sentinel& operator=(const move_sentinel<S2>& s) EA_NOEXCEPT_IF((eastl::is_nothrow_assignable<Sentinel&, const S2&>::value))
			{ mSentinel = s.base(); return *this; }

		EA_CONSTEXPR Sentinel base() const EA_NOEXCEPT_IF(eastl::is_nothrow_copy_constructible<Sentinel>::value)
			{ return mSentinel; }

	private:
		Sentinel mSentinel;
	};

	template <typename Iterator, typename Sentinel>
	inline internal::iterator_comparison_t<decltype(eastl::declval<const Iterator&>() == eastl::declval<const Sentinel&>())>
	operator==(const move_iterator<Iterator>& a, const move_sentinel<Sentinel>& b)
		EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(eastl::declval<const Iterator&>() == eastl::declval<const Sentinel&>()))
		{ return a.base() == b.base(); }

	#if !defined(EA_COMPILER_HAS_THREE_WAY_COMPARISON) // C++20 rewrites the other three from operator==.
	template <typename Iterator, typename Sentinel>
	inline internal::iterator_comparison_t<decltype(eastl::declval<const Iterator&>() == eastl::declval<const Sentinel&>())>
	operator==(const move_sentinel<Sentinel>& b, const move_iterator<Iterator>& a)
		EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(eastl::declval<const Iterator&>() == eastl::declval<const Sentinel&>()))
		{ return a.base() == b.base(); }

	template <typename Iterator, typename Sentinel>
	inline internal::iterator_comparison_t<decltype(eastl::declval<const Iterator&>() == eastl::declval<const Sentinel&>())>
	operator!=(const move_iterator<Iterator>& a, const move_sentinel<Sentinel>& b)
		EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(eastl::declval<const Iterator&>() == eastl::declval<const Sentinel&>()))
		{ return !(a.base() == b.base()); }

	template <typename Iterator, typename Sentinel>
	inline internal::iterator_comparison_t<decltype(eastl::declval<const Iterator&>() == eastl::declval<const Sentinel&>())>
	operator!=(const move_sentinel<Sentinel>& b, const move_iterator<Iterator>& a)
		EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(eastl::declval<const Iterator&>() == eastl::declval<const Sentinel&>()))
		{ return !(a.base() == b.base()); }
	#endif

	template <typename Iterator, typename Sentinel>
	inline auto operator-(const move_sentinel<Sentinel>& b, const move_iterator<Iterator>& a)
		EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(eastl::declval<const Sentinel&>() - eastl::declval<const Iterator&>())) -> decltype(b.base() - a.base())
		{ return b.base() - a.base(); }

	template <typename Iterator, typename Sentinel>
	inline auto operator-(const move_iterator<Iterator>& a, const move_sentinel<Sentinel>& b)
		EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(eastl::declval<const Iterator&>() - eastl::declval<const Sentinel&>())) -> decltype(a.base() - b.base())
		{ return a.base() - b.base(); }


	template<typename Iterator>
	inline move_iterator<Iterator>
	operator+(typename move_iterator<Iterator>::difference_type n, const move_iterator<Iterator>& a)
		EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(a + n))
		{ return a + n; }


	template<typename Iterator>
	inline move_iterator<Iterator> make_move_iterator(Iterator i) EA_NOEXCEPT_IF(eastl::is_nothrow_move_constructible<Iterator>::value)
		{ return move_iterator<Iterator>(i); }


	// make_move_if_noexcept_iterator returns move_iterator<Iterator> if the Iterator is of a noexcept type;
	// otherwise returns Iterator as-is. The point of this is to be able to avoid moves that can generate exceptions and instead
	// fall back to copies or whatever the default IteratorType::operator* returns for use by copy/move algorithms.
	// To consider: merge the conditional expression usage here with the one used by move_if_noexcept, as they are the same condition.
	#if EASTL_EXCEPTIONS_ENABLED
		template <typename Iterator, typename IteratorType = typename eastl::conditional<eastl::is_nothrow_move_constructible<typename eastl::iterator_traits<Iterator>::value_type>::value ||
																						 !eastl::is_copy_constructible<typename eastl::iterator_traits<Iterator>::value_type>::value,
																						 eastl::move_iterator<Iterator>, Iterator>::type>
		inline IteratorType make_move_if_noexcept_iterator(Iterator i)
			{ return IteratorType(i); }
	#else
		// Else there are no exceptions and thus we always return a move_iterator.
		template <typename Iterator>
		inline eastl::move_iterator<Iterator> make_move_if_noexcept_iterator(Iterator i)
			{ return eastl::move_iterator<Iterator>(i); }
	#endif



	/// is_move_iterator
	///
	/// This is a type traits extension utility.
	/// Given an iterator, tells if it's a move iterator vs anything else.
	/// Example usage (though somewhat useless):
	///     template <typename T>
	///     bool IsMoveIterator() { return typename eastl::is_move_iterator<T>::value; }
	///
	template <typename T>
	struct EASTL_REMOVE_AT_2024_SEPT is_move_iterator
		: public eastl::false_type {};

	// Note: some compilers (notably GCC) trigger deprecation warnings when doing template
	// specialization if the main template is derpecated, so turn the warning off here. If this
	// specialization is used, the warning will still trigger in the user code, this just
	// disables the warning in this declaration.
EASTL_INTERNAL_DISABLE_DEPRECATED()
	template<typename Iterator>
	struct EASTL_REMOVE_AT_2024_SEPT is_move_iterator<eastl::move_iterator<Iterator>>
		: public eastl::true_type {};
EASTL_INTERNAL_RESTORE_DEPRECATED()


	/// unwrap_move_iterator
	///
	/// Returns `it.base()` if it's a move_iterator, else returns `it` as-is.
	///
	/// Example usage:
	///      vector<int> intVector;
	///      eastl::move_iterator<vector<int>::iterator> moveIterator(intVector.begin());
	///      vector<int>::iterator it = unwrap_move_iterator(moveIterator);
	///
EASTL_INTERNAL_DISABLE_DEPRECATED() // is_iterator_wrapper_helper is deprecated
	template <typename Iterator>
	EASTL_REMOVE_AT_2024_SEPT inline typename eastl::is_iterator_wrapper_helper<Iterator, eastl::is_move_iterator<Iterator>::value>::iterator_type unwrap_move_iterator(Iterator it)
	{
		// get_unwrapped(it) -> it.unwrap() which is equivalent to `it.base()` for move_iterator and to `it` otherwise.
		return eastl::is_iterator_wrapper_helper<Iterator, eastl::is_move_iterator<Iterator>::value>::get_unwrapped(it);
	}
EASTL_INTERNAL_RESTORE_DEPRECATED()


	/// back_insert_iterator
	///
	/// A back_insert_iterator is simply a class that acts like an iterator but when you
	/// assign a value to it, it calls push_back on the container with the value.
	///
	template <typename Container>
	class back_insert_iterator
	{
	public:
		typedef back_insert_iterator<Container>     this_type;
		typedef Container                           container_type;
		typedef typename Container::const_reference const_reference;
		typedef eastl::output_iterator_tag	iterator_category;
		typedef void								value_type;
		typedef void								difference_type;
		typedef void								pointer;
		typedef void								reference;

	protected:
		Container* container;

	public:
		explicit back_insert_iterator(Container& x) EA_NOEXCEPT
			: container(eastl::addressof(x)) { }

		back_insert_iterator(const back_insert_iterator&) = default;
		back_insert_iterator& operator=(const back_insert_iterator&) = default;

		back_insert_iterator& operator=(const typename Container::value_type& value)
			{ container->push_back(value); return *this; }

		back_insert_iterator& operator=(typename Container::value_type&& value)
			{ container->push_back(eastl::move(value)); return *this; }

		back_insert_iterator& operator*() EA_NOEXCEPT
			{ return *this; }

		back_insert_iterator& operator++() EA_NOEXCEPT
			{ return *this; } // This is by design.

		back_insert_iterator operator++(int) EA_NOEXCEPT
			{ return *this; } // This is by design.
	};


	/// back_inserter
	///
	/// Creates an instance of a back_insert_iterator.
	///
	template <typename Container>
	inline back_insert_iterator<Container>
	back_inserter(Container& x) EA_NOEXCEPT
		{ return back_insert_iterator<Container>(x); }




	/// front_insert_iterator
	///
	/// A front_insert_iterator is simply a class that acts like an iterator but when you
	/// assign a value to it, it calls push_front on the container with the value.
	///
	template <typename Container>
	class front_insert_iterator
	{
	public:
		typedef front_insert_iterator<Container>    this_type;
		typedef Container                           container_type;
		typedef typename Container::const_reference const_reference;
		typedef eastl::output_iterator_tag	iterator_category;
		typedef void								value_type;
		typedef void								difference_type;
		typedef void								pointer;
		typedef void								reference;

	protected:
		Container* container;

	public:
		explicit front_insert_iterator(Container& x) EA_NOEXCEPT : container(eastl::addressof(x)) {}

		front_insert_iterator(const front_insert_iterator&) = default;
		front_insert_iterator& operator=(const front_insert_iterator&) = default;

		front_insert_iterator& operator=(const typename Container::value_type& value)
			{ container->push_front(value); return *this; }

		front_insert_iterator& operator*() EA_NOEXCEPT
			{ return *this; }

		front_insert_iterator& operator++() EA_NOEXCEPT
			{ return *this; } // This is by design.

		front_insert_iterator operator++(int) EA_NOEXCEPT
			{ return *this; } // This is by design.
	};


	/// front_inserter
	///
	/// Creates an instance of a front_insert_iterator.
	///
	template <typename Container>
	inline front_insert_iterator<Container>
	front_inserter(Container& x) EA_NOEXCEPT
		{ return front_insert_iterator<Container>(x); }




	/// insert_iterator
	///
	/// An insert_iterator is like an iterator except that when you assign a value to it,
	/// the insert_iterator inserts the value into the container and increments the iterator.
	///
	/// insert_iterator is an iterator adaptor that functions as an OutputIterator:
	/// assignment through an insert_iterator inserts an object into a container.
	/// Specifically, if ii is an insert_iterator, then ii keeps track of a container c and
	/// an insertion point p; the expression *ii = x performs the insertion container.insert(p, x).
	///
	/// If you assign through an insert_iterator several times, then you will be inserting
	/// several elements into the underlying container. In the case of a sequence, they will
	/// appear at a particular location in the underlying sequence, in the order in which
	/// they were inserted: one of the arguments to insert_iterator's constructor is an
	/// iterator p, and the new range will be inserted immediately before p.
	///
	template <typename Container>
	class insert_iterator
	{
	public:
		typedef Container                           container_type;
		typedef typename Container::iterator        iterator_type;
		typedef typename Container::const_reference const_reference;
		typedef eastl::output_iterator_tag	iterator_category;
		typedef void								value_type;
		typedef void								difference_type;
		typedef void								pointer;
		typedef void								reference;

	protected:
		Container*     container;
		iterator_type  it;

	public:
		insert_iterator(Container& x, iterator_type itNew)
			: container(eastl::addressof(x)), it(itNew) {}

		insert_iterator(const insert_iterator&) = default;
		insert_iterator& operator=(const insert_iterator&) = default;

		insert_iterator& operator=(const typename Container::value_type& value)
		{
			it = container->insert(it, value);
			++it;
			return *this;
		}

		insert_iterator& operator*() EA_NOEXCEPT
			{ return *this; }

		insert_iterator& operator++() EA_NOEXCEPT
			{ return *this; } // This is by design.

		insert_iterator& operator++(int) EA_NOEXCEPT
			{ return *this; } // This is by design.

	}; // insert_iterator


	/// inserter
	///
	/// Creates an instance of an insert_iterator.
	///
	template <typename Container, typename Iterator>
	inline eastl::insert_iterator<Container>
	inserter(Container& x, Iterator i)
	{
		typedef typename Container::iterator iterator;
		return eastl::insert_iterator<Container>(x, iterator(i));
	}


	/// is_insert_iterator
	///
	/// This is a type traits extension utility.
	/// Given an iterator, tells if it's an insert_iterator vs anything else.
	/// If it's a insert_iterator wrapped by another iterator then value is false.
	///
	template <typename T>
	struct EASTL_REMOVE_AT_2024_SEPT is_insert_iterator
		: public eastl::false_type {};

	// Note: some compilers (notably GCC) trigger deprecation warnings when doing template
	// specialization if the main template is derpecated, so turn the warning off here. If this
	// specialization is used, the warning will still trigger in the user code, this just
	// disables the warning in this declaration.
EASTL_INTERNAL_DISABLE_DEPRECATED()
	template<typename Iterator>
	struct EASTL_REMOVE_AT_2024_SEPT is_insert_iterator<eastl::insert_iterator<Iterator>>
		: public eastl::true_type {};
EASTL_INTERNAL_RESTORE_DEPRECATED()




	//////////////////////////////////////////////////////////////////////////////////
	/// distance
	///
	/// Implements the distance() function. There are two versions, one for
	/// random access iterators (e.g. with vector) and one for regular input
	/// iterators (e.g. with list). The former is more efficient.
	///
	template <typename InputIterator>
	EA_CONSTEXPR
	inline typename eastl::iterator_traits<InputIterator>::difference_type
	distance_impl(InputIterator first, InputIterator last, eastl::input_iterator_tag)
	{
		typename eastl::iterator_traits<InputIterator>::difference_type n = 0;

		while(first != last)
		{
			++first;
			++n;
		}
		return n;
	}

	template <typename RandomAccessIterator>
	EA_CONSTEXPR
	inline typename eastl::iterator_traits<RandomAccessIterator>::difference_type
	distance_impl(RandomAccessIterator first, RandomAccessIterator last, eastl::random_access_iterator_tag)
	{
		return last - first;
	}

	// Special version defined so that std C++ iterators can be recognized by
	// this function. Unfortunately, this function treats all foreign iterators
	// as InputIterators and thus can seriously hamper performance in the case
	// of large ranges of bidirectional_iterator_tag iterators.
	//template <typename InputIterator>
	//inline typename eastl::iterator_traits<InputIterator>::difference_type
	//distance_impl(InputIterator first, InputIterator last, ...)
	//{
	//    typename eastl::iterator_traits<InputIterator>::difference_type n = 0;
	//
	//    while(first != last)
	//    {
	//        ++first;
	//        ++n;
	//    }
	//    return n;
	//}

	template <typename InputIterator>
	EA_CONSTEXPR
	inline typename eastl::iterator_traits<InputIterator>::difference_type
	distance(InputIterator first, InputIterator last)
	{
		typedef typename eastl::iterator_traits<InputIterator>::iterator_category IC;

		return eastl::distance_impl(first, last, IC());
	}




	//////////////////////////////////////////////////////////////////////////////////
	/// advance
	///
	/// Implements the advance() function. There are three versions, one for
	/// random access iterators (e.g. with vector), one for bidirectional
	/// iterators (list) and one for regular input iterators (e.g. with slist).
	///
	template <typename InputIterator, typename Distance>
	EA_CPP14_CONSTEXPR inline void
	advance_impl(InputIterator& i, Distance n, eastl::input_iterator_tag)
	{
		while(n--)
			++i;
	}

	template <bool signedDistance>
	struct advance_bi_impl
	{
		template <typename BidirectionalIterator, typename Distance>
		static EA_CPP14_CONSTEXPR void advance_impl(BidirectionalIterator& i, Distance n) // Specialization for unsigned distance type.
		{
			while(n--)
				++i;
		}
	};

	template <>
	struct advance_bi_impl<true>
	{
		template <typename BidirectionalIterator, typename Distance>
		static EA_CPP14_CONSTEXPR void advance_impl(BidirectionalIterator& i, Distance n) // Specialization for signed distance type.
		{
			if(n > 0)
			{
				while(n--)
					++i;
			}
			else
			{
				while(n++)
					--i;
			}
		}
	};

	template <typename BidirectionalIterator, typename Distance>
	EA_CPP14_CONSTEXPR inline void
	advance_impl(BidirectionalIterator& i, Distance n, eastl::bidirectional_iterator_tag)
	{
		advance_bi_impl<eastl::is_signed<Distance>::value>::advance_impl(i, n);
	}

	template <typename RandomAccessIterator, typename Distance>
	EA_CPP14_CONSTEXPR inline void
	advance_impl(RandomAccessIterator& i, Distance n, eastl::random_access_iterator_tag)
	{
		i += n;
	}

	// Special version defined so that std C++ iterators can be recognized by
	// this function. Unfortunately, this function treats all foreign iterators
	// as InputIterators and thus can seriously hamper performance in the case
	// of large ranges of bidirectional_iterator_tag iterators.
	//template <typename InputIterator, typename Distance>
	//inline void
	//advance_impl(InputIterator& i, Distance n, ...)
	//{
	//    while(n--)
	//        ++i;
	//}

	template <typename InputIterator, typename Distance>
	EA_CPP14_CONSTEXPR inline void
	advance(InputIterator& i, Distance n)
	{
		typedef typename eastl::iterator_traits<InputIterator>::iterator_category IC;

		eastl::advance_impl(i, n, IC());
	}


	// eastl::next / eastl::prev
	// Return the nth/-nth successor of iterator it.
	//
	// http://en.cppreference.com/w/cpp/iterator/next
	//
	template<typename InputIterator>
	inline InputIterator
	next(InputIterator it, typename eastl::iterator_traits<InputIterator>::difference_type n = 1)
	{
		eastl::advance(it, n);
		return it;
	}

	template<typename InputIterator>
	inline InputIterator
	prev(InputIterator it, typename eastl::iterator_traits<InputIterator>::difference_type n = 1)
	{
		eastl::advance(it, -n);
		return it;
	}


#if defined(EA_COMPILER_CPP11_ENABLED) && EA_COMPILER_CPP11_ENABLED

	// eastl::data
	//
	// http://en.cppreference.com/w/cpp/iterator/data
	//
	template <class Container>
	EA_CPP14_CONSTEXPR auto data(Container& c) EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(c.data())) -> decltype(c.data())
		{ return c.data(); }

	template <class Container>
	EA_CPP14_CONSTEXPR auto data(const Container& c) EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(c.data())) -> decltype(c.data())
		{ return c.data(); }

	template <class T, size_t N>
	EA_CPP14_CONSTEXPR T* data(T(&array)[N]) EA_NOEXCEPT
		{ return array; }

	template <class E>
	EA_CPP14_CONSTEXPR const E* data(std::initializer_list<E> il) EA_NOEXCEPT
		{ return il.begin(); }


	// eastl::size
	//
	// http://en.cppreference.com/w/cpp/iterator/size
	//
	template <class C>
	EA_CPP14_CONSTEXPR auto size(const C& c) EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(c.size())) -> decltype(c.size())
		{ return c.size(); }

	template <class T, size_t N>
	EA_CPP14_CONSTEXPR size_t size(const T (&)[N]) EA_NOEXCEPT
		{ return N; }


	// eastl::ssize
	//
	// https://en.cppreference.com/w/cpp/iterator/size
	//
	template <class T, ptrdiff_t N>
	EA_CPP14_CONSTEXPR ptrdiff_t ssize(const T(&)[N]) EA_NOEXCEPT
		{ return N; }

	template <class C>
	EA_CPP14_CONSTEXPR auto ssize(const C& c) EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(c.size()))
	    -> eastl::common_type_t<ptrdiff_t, eastl::make_signed_t<decltype(c.size())>>
	{
		using R = eastl::common_type_t<ptrdiff_t, eastl::make_signed_t<decltype(c.size())>>;
		return static_cast<R>(c.size());
	}


	// eastl::empty
	//
	// http://en.cppreference.com/w/cpp/iterator/empty
	//
	template <class Container>
	EA_CPP14_CONSTEXPR auto empty(const Container& c) EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(c.empty())) -> decltype(c.empty())
		{ return c.empty(); }

	template <class T, size_t N>
	EA_CPP14_CONSTEXPR bool empty(const T (&)[N]) EA_NOEXCEPT
		{ return false; }

	template <class E>
	EA_CPP14_CONSTEXPR bool empty(std::initializer_list<E> il) EA_NOEXCEPT
		{ return il.size() == 0; }

#endif // defined(EA_COMPILER_CPP11_ENABLED) && EA_COMPILER_CPP11_ENABLED


	// eastl::begin / eastl::end
	// http://en.cppreference.com/w/cpp/iterator/begin
	//
	// In order to enable eastl::begin and eastl::end, the compiler needs to have conforming support
	// for argument-dependent lookup if it supports C++11 range-based for loops. The reason for this is
	// that in C++11 range-based for loops result in usage of std::begin/std::end, but allow that to
	// be overridden by argument-dependent lookup:
	//     C++11 Standard, section 6.5.4, paragraph 1.
	//     "otherwise, begin-expr and end-expr are begin(__range) and end(__range), respectively,
	//      where begin and end are looked up with argument-dependent lookup (3.4.2). For the
	//      purposes of this name lookup, namespace std is an associated namespace."
	// It turns out that one compiler has a problem: GCC 4.6. That version added support for
	// range-based for loops but has broken argument-dependent lookup which was fixed in GCC 4.7.
	//
	#if (defined(EA_COMPILER_GNUC) && (EA_COMPILER_VERSION == 4006))
		#define EASTL_BEGIN_END_ENABLED 0
	#else
		#define EASTL_BEGIN_END_ENABLED 1
	#endif

	#if EASTL_BEGIN_END_ENABLED
		template <typename Container>
		EA_CPP14_CONSTEXPR inline auto begin(Container& container) EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(container.begin())) -> decltype(container.begin())
		{
			return container.begin();
		}

		template <typename Container>
		EA_CPP14_CONSTEXPR inline auto begin(const Container& container) EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(container.begin())) -> decltype(container.begin())
		{
			return container.begin();
		}

		template<typename T, size_t arraySize>
		EA_CPP14_CONSTEXPR inline T* begin(T (&arrayObject)[arraySize]) EA_NOEXCEPT
		{
			return arrayObject;
		}

		template <typename Container>
		EA_CPP14_CONSTEXPR inline auto cbegin(const Container& container) EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(eastl::begin(container))) -> decltype(eastl::begin(container))
		{
			return eastl::begin(container);
		}

		template <typename Container>
		EA_CPP14_CONSTEXPR inline auto end(Container& container) EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(container.end())) -> decltype(container.end())
		{
			return container.end();
		}

		template <typename Container>
		EA_CPP14_CONSTEXPR inline auto end(const Container& container) EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(container.end())) -> decltype(container.end())
		{
			return container.end();
		}

		template<typename T, size_t arraySize>
		EA_CPP14_CONSTEXPR inline T* end(T (&arrayObject)[arraySize]) EA_NOEXCEPT
		{
			return (arrayObject + arraySize);
		}

		template <typename Container>
		EA_CPP14_CONSTEXPR inline auto cend(const Container& container) EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(eastl::end(container))) -> decltype(eastl::end(container))
		{
			return eastl::end(container);
		}

		template <typename Container>
		EA_CPP14_CONSTEXPR inline auto rbegin(Container& container) EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(container.rbegin())) -> decltype(container.rbegin())
		{
			return container.rbegin();
		}

		template <typename Container>
		EA_CPP14_CONSTEXPR inline auto rbegin(const Container& container) EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(container.rbegin())) -> decltype(container.rbegin())
		{
			return container.rbegin();
		}

		template <typename Container>
		EA_CPP14_CONSTEXPR inline auto rend(Container& container) EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(container.rend())) -> decltype(container.rend())
		{
			return container.rend();
		}

		template <typename Container>
		EA_CPP14_CONSTEXPR inline auto rend(const Container& container) EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(container.rend())) -> decltype(container.rend())
		{
			return container.rend();
		}

		template <typename Container>
		EA_CPP14_CONSTEXPR inline auto crbegin(const Container& container) EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(eastl::rbegin(container))) -> decltype(eastl::rbegin(container))
		{
			return container.rbegin();
		}

		template <typename Container>
		EA_CPP14_CONSTEXPR inline auto crend(const Container& container) EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(eastl::rend(container))) -> decltype(eastl::rend(container))
		{
			return container.rend();
		}


		template <typename T, size_t arraySize>
		EA_CPP14_CONSTEXPR inline reverse_iterator<T*> rbegin(T (&arrayObject)[arraySize]) EA_NOEXCEPT
		{
			return reverse_iterator<T*>(arrayObject + arraySize);
		}

		template <typename T, size_t arraySize>
		EA_CPP14_CONSTEXPR inline reverse_iterator<T*> rend(T (&arrayObject)[arraySize]) EA_NOEXCEPT
		{
			return reverse_iterator<T*>(arrayObject);
		}

		template <typename E>
		EA_CPP14_CONSTEXPR inline reverse_iterator<const E*> rbegin(std::initializer_list<E> ilist) EA_NOEXCEPT
		{
			return eastl::reverse_iterator<const E*>(ilist.end());
		}

		template <typename E>
		EA_CPP14_CONSTEXPR inline reverse_iterator<const E*> rend(std::initializer_list<E> ilist) EA_NOEXCEPT
		{
			return eastl::reverse_iterator<const E*>(ilist.begin());
		}

		template <typename Iterator>
		EA_CPP14_CONSTEXPR reverse_iterator<Iterator> make_reverse_iterator(Iterator i) EA_NOEXCEPT_IF(eastl::is_nothrow_move_constructible<Iterator>::value)
			{ return reverse_iterator<Iterator>(i); }

	#endif // EASTL_BEGIN_END_ENABLED

} // namespace eastl



// Some compilers (e.g. GCC 4.6) support range-based for loops, but have a bug with
// respect to argument-dependent lookup which results on them unilaterally using std::begin/end
// with range-based for loops. To work around this we #include <iterator> for this case in
// order to make std::begin/end visible to users of <EASTL/iterator.h>, for portability.
#if !EASTL_BEGIN_END_ENABLED && !defined(EA_COMPILER_NO_RANGE_BASED_FOR_LOOP)
	#include <iterator>
#endif



EA_RESTORE_VC_WARNING();
EA_RESTORE_VC_WARNING();

#endif // Header include guard
