/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#include "EASTLTest.h"
#include <EASTL/deque.h>
#include <EASTL/iterator.h>
#include <EASTL/vector.h>
#include <EASTL/set.h>
#include <EASTL/array.h>
#include <EASTL/numeric.h>
#include <EASTL/list.h>
#include <EASTL/slist.h>
#include <EASTL/string.h>
#include <EASTL/intrusive_list.h>
#include <EASTL/memory.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/algorithm.h>
#include <EASTL/internal/generic_iterator.h>

// The exception specifications of the iterator adaptors, the inserters and the range access functions follow the
// standard (the conditional ones as MSVC's STL strengthens them).
namespace NoexceptIteratorTest
{
	template <bool Nothrow>
	struct Iter
	{
		typedef eastl::random_access_iterator_tag iterator_category;
		typedef int                               value_type;
		typedef ptrdiff_t                         difference_type;
		typedef int*                              pointer;
		typedef int&                              reference;

		Iter() EA_NOEXCEPT_IF(Nothrow);
		Iter(const Iter&) EA_NOEXCEPT_IF(Nothrow);
		Iter& operator=(const Iter&) EA_NOEXCEPT_IF(Nothrow);
		int& operator*() const EA_NOEXCEPT_IF(Nothrow);
		int* operator->() const EA_NOEXCEPT_IF(Nothrow);
		Iter& operator++() EA_NOEXCEPT_IF(Nothrow);
		Iter operator++(int) EA_NOEXCEPT_IF(Nothrow);
		Iter& operator--() EA_NOEXCEPT_IF(Nothrow);
		Iter operator--(int) EA_NOEXCEPT_IF(Nothrow);
		Iter& operator+=(ptrdiff_t) EA_NOEXCEPT_IF(Nothrow);
		Iter& operator-=(ptrdiff_t) EA_NOEXCEPT_IF(Nothrow);
		Iter operator+(ptrdiff_t) const EA_NOEXCEPT_IF(Nothrow);
		Iter operator-(ptrdiff_t) const EA_NOEXCEPT_IF(Nothrow);
		int& operator[](ptrdiff_t) const EA_NOEXCEPT_IF(Nothrow);
		friend ptrdiff_t operator-(const Iter&, const Iter&) EA_NOEXCEPT_IF(Nothrow);
		friend bool operator==(const Iter&, const Iter&) EA_NOEXCEPT_IF(Nothrow);
		friend bool operator!=(const Iter&, const Iter&) EA_NOEXCEPT_IF(Nothrow);
		friend bool operator<(const Iter&, const Iter&) EA_NOEXCEPT_IF(Nothrow);
		friend bool operator>(const Iter&, const Iter&) EA_NOEXCEPT_IF(Nothrow);
		friend bool operator<=(const Iter&, const Iter&) EA_NOEXCEPT_IF(Nothrow);
		friend bool operator>=(const Iter&, const Iter&) EA_NOEXCEPT_IF(Nothrow);
	};
	typedef Iter<true>  NothrowIter;
	typedef Iter<false> ThrowingIter;

	// Converts to Iter<Nothrow> through its copy constructor.
	template <bool Nothrow>
	struct ConvertibleIter : public Iter<Nothrow> {};

	// The exception specifications that depend on the base iterator, with a nothrow iterator (I) and an iterator that converts to it (J).
	#define EASTL_TEST_NOEXCEPT_ITER(AdaptorE, I, J) \
		static_assert(noexcept(AdaptorE<I>()), #AdaptorE "() is conditionally noexcept"); \
		static_assert(noexcept(AdaptorE<I>(eastl::declval<I>())), #AdaptorE "(iterator) is conditionally noexcept"); \
		static_assert(noexcept(AdaptorE<I>(eastl::declval<const AdaptorE<J>&>())), #AdaptorE "(const adaptor<J>&) is conditionally noexcept"); \
		static_assert(noexcept(eastl::declval<AdaptorE<I>&>() = eastl::declval<const AdaptorE<J>&>()), #AdaptorE "::operator=(const adaptor<J>&) is conditionally noexcept"); \
		static_assert(noexcept(eastl::declval<const AdaptorE<I>&>().operator->()), #AdaptorE "::operator-> is conditionally noexcept"); \
		static_assert(noexcept(++eastl::declval<AdaptorE<I>&>()), #AdaptorE "::operator++ is conditionally noexcept"); \
		static_assert(noexcept(eastl::declval<AdaptorE<I>&>()++), #AdaptorE "::operator++(int) is conditionally noexcept"); \
		static_assert(noexcept(--eastl::declval<AdaptorE<I>&>()), #AdaptorE "::operator-- is conditionally noexcept"); \
		static_assert(noexcept(eastl::declval<AdaptorE<I>&>()--), #AdaptorE "::operator--(int) is conditionally noexcept"); \
		static_assert(noexcept(eastl::declval<const AdaptorE<I>&>() + 1), #AdaptorE "::operator+ is conditionally noexcept"); \
		static_assert(noexcept(eastl::declval<AdaptorE<I>&>() += 1), #AdaptorE "::operator+= is conditionally noexcept"); \
		static_assert(noexcept(eastl::declval<const AdaptorE<I>&>() - 1), #AdaptorE "::operator- is conditionally noexcept"); \
		static_assert(noexcept(eastl::declval<AdaptorE<I>&>() -= 1), #AdaptorE "::operator-= is conditionally noexcept"); \
		static_assert(noexcept(eastl::declval<const AdaptorE<I>&>() - eastl::declval<const AdaptorE<I>&>()), #AdaptorE " difference is conditionally noexcept"); \
		static_assert(noexcept(1 + eastl::declval<const AdaptorE<I>&>()), "n + " #AdaptorE " is conditionally noexcept"); \
		static_assert(noexcept(eastl::declval<const AdaptorE<I>&>() == eastl::declval<const AdaptorE<I>&>()), #AdaptorE " == is conditionally noexcept"); \
		static_assert(noexcept(eastl::declval<const AdaptorE<I>&>() != eastl::declval<const AdaptorE<I>&>()), #AdaptorE " != is conditionally noexcept"); \
		static_assert(noexcept(eastl::declval<const AdaptorE<I>&>() < eastl::declval<const AdaptorE<I>&>()), #AdaptorE " < is conditionally noexcept"); \
		static_assert(noexcept(eastl::declval<const AdaptorE<I>&>() > eastl::declval<const AdaptorE<I>&>()), #AdaptorE " > is conditionally noexcept"); \
		static_assert(noexcept(eastl::declval<const AdaptorE<I>&>() <= eastl::declval<const AdaptorE<I>&>()), #AdaptorE " <= is conditionally noexcept"); \
		static_assert(noexcept(eastl::declval<const AdaptorE<I>&>() >= eastl::declval<const AdaptorE<I>&>()), #AdaptorE " >= is conditionally noexcept");

	// reverse_iterator, with a nothrow iterator and with a pointer.
	EASTL_TEST_NOEXCEPT_ITER(eastl::reverse_iterator, NothrowIter, ConvertibleIter<true>)
	static_assert(noexcept(eastl::reverse_iterator<int*>()), "reverse_iterator<int*>() is noexcept");
	static_assert(noexcept(eastl::declval<const eastl::reverse_iterator<NothrowIter>&>().base()), "reverse_iterator::base is noexcept for a nothrow iterator");
	static_assert(noexcept(*eastl::declval<const eastl::reverse_iterator<NothrowIter>&>()), "reverse_iterator::operator* is noexcept for a nothrow iterator");
	static_assert(noexcept(eastl::declval<const eastl::reverse_iterator<NothrowIter>&>()[1]), "reverse_iterator::operator[] is noexcept for a nothrow iterator");
	static_assert(noexcept(eastl::make_reverse_iterator(eastl::declval<NothrowIter>())), "make_reverse_iterator is noexcept for a nothrow iterator");
	static_assert(noexcept(eastl::make_reverse_iterator(eastl::declval<int*>())), "make_reverse_iterator is noexcept for a pointer");
	static_assert(noexcept(eastl::declval<const eastl::reverse_iterator<int*>&>().operator->()), "reverse_iterator<int*>::operator-> is noexcept");

	// ... and with an iterator that may throw: the adaptor's operations may throw too.
	static_assert(!noexcept(eastl::reverse_iterator<ThrowingIter>()), "reverse_iterator<ThrowingIter>() may throw");
	static_assert(!noexcept(eastl::reverse_iterator<ThrowingIter>(eastl::declval<ThrowingIter>())), "reverse_iterator<ThrowingIter>(iterator) may throw");
	static_assert(!noexcept(eastl::declval<const eastl::reverse_iterator<ThrowingIter>&>().base()), "reverse_iterator::base may throw for a throwing iterator");
	static_assert(!noexcept(*eastl::declval<const eastl::reverse_iterator<ThrowingIter>&>()), "reverse_iterator::operator* may throw for a throwing iterator");
	static_assert(!noexcept(++eastl::declval<eastl::reverse_iterator<ThrowingIter>&>()), "reverse_iterator::operator++ may throw for a throwing iterator");
	static_assert(!noexcept(eastl::declval<eastl::reverse_iterator<ThrowingIter>&>() += 1), "reverse_iterator::operator+= may throw for a throwing iterator");
	static_assert(!noexcept(eastl::declval<const eastl::reverse_iterator<ThrowingIter>&>()[1]), "reverse_iterator::operator[] may throw for a throwing iterator");
	static_assert(!noexcept(eastl::declval<const eastl::reverse_iterator<ThrowingIter>&>() == eastl::declval<const eastl::reverse_iterator<ThrowingIter>&>()), "reverse_iterator == may throw for a throwing iterator");
	static_assert(!noexcept(eastl::declval<const eastl::reverse_iterator<ThrowingIter>&>() - eastl::declval<const eastl::reverse_iterator<ThrowingIter>&>()), "reverse_iterator difference may throw for a throwing iterator");
	static_assert(!noexcept(eastl::make_reverse_iterator(eastl::declval<ThrowingIter>())), "make_reverse_iterator may throw for a throwing iterator");
	#undef EASTL_TEST_NOEXCEPT_ITER

	// move_iterator.
	static_assert(noexcept(eastl::move_iterator<NothrowIter>()), "move_iterator() is conditionally noexcept");
	static_assert(noexcept(eastl::move_iterator<NothrowIter>(eastl::declval<NothrowIter>())), "move_iterator(iterator) is conditionally noexcept");
	static_assert(noexcept(eastl::move_iterator<NothrowIter>(eastl::declval<const eastl::move_iterator<ConvertibleIter<true>>&>())), "move_iterator(const move_iterator<U>&) is conditionally noexcept");
	static_assert(noexcept(eastl::declval<eastl::move_iterator<NothrowIter>&>() = eastl::declval<const eastl::move_iterator<ConvertibleIter<true>>&>()), "move_iterator::operator=(const move_iterator<U>&) is conditionally noexcept");
	static_assert(noexcept(eastl::declval<const eastl::move_iterator<NothrowIter>&>().base()), "move_iterator::base is noexcept");
	static_assert(noexcept(eastl::declval<const eastl::move_iterator<NothrowIter>&>().operator->()), "move_iterator::operator-> is conditionally noexcept");
	static_assert(noexcept(++eastl::declval<eastl::move_iterator<NothrowIter>&>()), "move_iterator::operator++ is conditionally noexcept");
	static_assert(noexcept(eastl::declval<eastl::move_iterator<NothrowIter>&>()++), "move_iterator::operator++(int) is conditionally noexcept");
	static_assert(noexcept(--eastl::declval<eastl::move_iterator<NothrowIter>&>()), "move_iterator::operator-- is conditionally noexcept");
	static_assert(noexcept(eastl::declval<eastl::move_iterator<NothrowIter>&>()--), "move_iterator::operator--(int) is conditionally noexcept");
	static_assert(noexcept(eastl::declval<const eastl::move_iterator<NothrowIter>&>() + 1), "move_iterator::operator+ is conditionally noexcept");
	static_assert(noexcept(eastl::declval<eastl::move_iterator<NothrowIter>&>() += 1), "move_iterator::operator+= is conditionally noexcept");
	static_assert(noexcept(eastl::declval<const eastl::move_iterator<NothrowIter>&>() - 1), "move_iterator::operator- is conditionally noexcept");
	static_assert(noexcept(eastl::declval<eastl::move_iterator<NothrowIter>&>() -= 1), "move_iterator::operator-= is conditionally noexcept");
	static_assert(noexcept(eastl::declval<const eastl::move_iterator<NothrowIter>&>() - eastl::declval<const eastl::move_iterator<NothrowIter>&>()), "move_iterator difference is conditionally noexcept");
	static_assert(noexcept(1 + eastl::declval<const eastl::move_iterator<NothrowIter>&>()), "n + move_iterator is conditionally noexcept");
	static_assert(noexcept(eastl::declval<const eastl::move_iterator<NothrowIter>&>() == eastl::declval<const eastl::move_iterator<NothrowIter>&>()), "move_iterator == is conditionally noexcept");
	static_assert(noexcept(eastl::declval<const eastl::move_iterator<NothrowIter>&>() != eastl::declval<const eastl::move_iterator<NothrowIter>&>()), "move_iterator != is conditionally noexcept");
	static_assert(noexcept(eastl::declval<const eastl::move_iterator<NothrowIter>&>() < eastl::declval<const eastl::move_iterator<NothrowIter>&>()), "move_iterator < is conditionally noexcept");
	static_assert(noexcept(eastl::declval<const eastl::move_iterator<NothrowIter>&>() > eastl::declval<const eastl::move_iterator<NothrowIter>&>()), "move_iterator > is conditionally noexcept");
	static_assert(noexcept(eastl::declval<const eastl::move_iterator<NothrowIter>&>() <= eastl::declval<const eastl::move_iterator<NothrowIter>&>()), "move_iterator <= is conditionally noexcept");
	static_assert(noexcept(eastl::declval<const eastl::move_iterator<NothrowIter>&>() >= eastl::declval<const eastl::move_iterator<NothrowIter>&>()), "move_iterator >= is conditionally noexcept");
	static_assert(noexcept(eastl::make_move_iterator(eastl::declval<NothrowIter>())), "make_move_iterator is conditionally noexcept");
	static_assert(noexcept(eastl::move_iterator<int*>()), "move_iterator<int*>() is noexcept");

	static_assert(!noexcept(eastl::move_iterator<ThrowingIter>()), "move_iterator<ThrowingIter>() may throw");
	static_assert(!noexcept(eastl::move_iterator<ThrowingIter>(eastl::declval<ThrowingIter>())), "move_iterator<ThrowingIter>(iterator) may throw");
	static_assert(!noexcept(++eastl::declval<eastl::move_iterator<ThrowingIter>&>()), "move_iterator::operator++ may throw for a throwing iterator");
	static_assert(!noexcept(eastl::declval<eastl::move_iterator<ThrowingIter>&>() += 1), "move_iterator::operator+= may throw for a throwing iterator");
	static_assert(!noexcept(eastl::declval<const eastl::move_iterator<ThrowingIter>&>() == eastl::declval<const eastl::move_iterator<ThrowingIter>&>()), "move_iterator == may throw for a throwing iterator");
	static_assert(!noexcept(eastl::declval<const eastl::move_iterator<ThrowingIter>&>() - eastl::declval<const eastl::move_iterator<ThrowingIter>&>()), "move_iterator difference may throw for a throwing iterator");
	static_assert(!noexcept(eastl::make_move_iterator(eastl::declval<ThrowingIter>())), "make_move_iterator may throw for a throwing iterator");

	// move_sentinel.
	static_assert(noexcept(eastl::move_sentinel<NothrowIter>()), "move_sentinel() is conditionally noexcept");
	static_assert(noexcept(eastl::move_sentinel<NothrowIter>(eastl::declval<NothrowIter>())), "move_sentinel(sentinel) is conditionally noexcept");
	static_assert(noexcept(eastl::move_sentinel<NothrowIter>(eastl::declval<const eastl::move_sentinel<ConvertibleIter<true>>&>())), "move_sentinel(const move_sentinel<S2>&) is conditionally noexcept");
	static_assert(noexcept(eastl::declval<eastl::move_sentinel<NothrowIter>&>() = eastl::declval<const eastl::move_sentinel<ConvertibleIter<true>>&>()), "move_sentinel::operator=(const move_sentinel<S2>&) is conditionally noexcept");
	static_assert(noexcept(eastl::declval<const eastl::move_sentinel<NothrowIter>&>().base()), "move_sentinel::base is conditionally noexcept");
	static_assert(noexcept(eastl::declval<const eastl::move_iterator<NothrowIter>&>() == eastl::declval<const eastl::move_sentinel<NothrowIter>&>()), "move_iterator == move_sentinel is conditionally noexcept");
	static_assert(noexcept(eastl::declval<const eastl::move_sentinel<NothrowIter>&>() - eastl::declval<const eastl::move_iterator<NothrowIter>&>()), "move_sentinel - move_iterator is conditionally noexcept");
	static_assert(noexcept(eastl::declval<const eastl::move_iterator<NothrowIter>&>() - eastl::declval<const eastl::move_sentinel<NothrowIter>&>()), "move_iterator - move_sentinel is conditionally noexcept");
	static_assert(!noexcept(eastl::move_sentinel<ThrowingIter>()), "move_sentinel<ThrowingIter>() may throw");
	static_assert(!noexcept(eastl::declval<const eastl::move_sentinel<ThrowingIter>&>().base()), "move_sentinel::base may throw for a throwing sentinel");
	static_assert(!noexcept(eastl::declval<const eastl::move_iterator<ThrowingIter>&>() == eastl::declval<const eastl::move_sentinel<ThrowingIter>&>()), "move_iterator == move_sentinel may throw for a throwing iterator");

	// back_insert_iterator and back_inserter.
	static_assert(noexcept(eastl::back_insert_iterator<eastl::vector<int> >(eastl::declval<eastl::vector<int>&>())), "back_insert_iterator(container) is noexcept");
	static_assert(noexcept(*eastl::declval<eastl::back_insert_iterator<eastl::vector<int> >&>()), "back_insert_iterator::operator* is noexcept");
	static_assert(noexcept(++eastl::declval<eastl::back_insert_iterator<eastl::vector<int> >&>()), "back_insert_iterator::operator++ is noexcept");
	static_assert(noexcept(eastl::declval<eastl::back_insert_iterator<eastl::vector<int> >&>()++), "back_insert_iterator::operator++(int) is noexcept");
	static_assert(noexcept(eastl::back_inserter(eastl::declval<eastl::vector<int>&>())), "back_inserter is noexcept");

	// Range access functions: noexcept when the container's member is.
	template <bool Nothrow>
	struct Range
	{
		int* begin() EA_NOEXCEPT_IF(Nothrow);
		int* end() EA_NOEXCEPT_IF(Nothrow);
		const int* begin() const EA_NOEXCEPT_IF(Nothrow);
		const int* end() const EA_NOEXCEPT_IF(Nothrow);
		eastl::reverse_iterator<int*> rbegin() EA_NOEXCEPT_IF(Nothrow);
		eastl::reverse_iterator<int*> rend() EA_NOEXCEPT_IF(Nothrow);
		eastl::reverse_iterator<const int*> rbegin() const EA_NOEXCEPT_IF(Nothrow);
		eastl::reverse_iterator<const int*> rend() const EA_NOEXCEPT_IF(Nothrow);
		size_t size() const EA_NOEXCEPT_IF(Nothrow);
		bool empty() const EA_NOEXCEPT_IF(Nothrow);
		int* data() EA_NOEXCEPT_IF(Nothrow);
		const int* data() const EA_NOEXCEPT_IF(Nothrow);
	};

	#define EASTL_TEST_NOEXCEPT_RANGE(R, Expected) \
		static_assert(noexcept(eastl::begin(eastl::declval<R&>())) == Expected, "begin(R&) exception specification"); \
		static_assert(noexcept(eastl::begin(eastl::declval<const R&>())) == Expected, "begin(const R&) exception specification"); \
		static_assert(noexcept(eastl::cbegin(eastl::declval<R&>())) == Expected, "cbegin exception specification"); \
		static_assert(noexcept(eastl::end(eastl::declval<R&>())) == Expected, "end(R&) exception specification"); \
		static_assert(noexcept(eastl::end(eastl::declval<const R&>())) == Expected, "end(const R&) exception specification"); \
		static_assert(noexcept(eastl::cend(eastl::declval<R&>())) == Expected, "cend exception specification"); \
		static_assert(noexcept(eastl::rbegin(eastl::declval<R&>())) == Expected, "rbegin(R&) exception specification"); \
		static_assert(noexcept(eastl::rbegin(eastl::declval<const R&>())) == Expected, "rbegin(const R&) exception specification"); \
		static_assert(noexcept(eastl::crbegin(eastl::declval<R&>())) == Expected, "crbegin exception specification"); \
		static_assert(noexcept(eastl::rend(eastl::declval<R&>())) == Expected, "rend(R&) exception specification"); \
		static_assert(noexcept(eastl::rend(eastl::declval<const R&>())) == Expected, "rend(const R&) exception specification"); \
		static_assert(noexcept(eastl::crend(eastl::declval<R&>())) == Expected, "crend exception specification"); \
		static_assert(noexcept(eastl::size(eastl::declval<const R&>())) == Expected, "size exception specification"); \
		static_assert(noexcept(eastl::ssize(eastl::declval<const R&>())) == Expected, "ssize exception specification"); \
		static_assert(noexcept(eastl::empty(eastl::declval<const R&>())) == Expected, "empty exception specification"); \
		static_assert(noexcept(eastl::data(eastl::declval<R&>())) == Expected, "data(R&) exception specification"); \
		static_assert(noexcept(eastl::data(eastl::declval<const R&>())) == Expected, "data(const R&) exception specification");
	EASTL_TEST_NOEXCEPT_RANGE(Range<true>, true)
	EASTL_TEST_NOEXCEPT_RANGE(Range<false>, false)
	#undef EASTL_TEST_NOEXCEPT_RANGE

	static_assert(noexcept(eastl::rbegin(eastl::declval<int (&)[3]>())), "rbegin(array) is noexcept");
	static_assert(noexcept(eastl::rend(eastl::declval<int (&)[3]>())), "rend(array) is noexcept");
	static_assert(noexcept(eastl::rbegin(eastl::declval<std::initializer_list<int> >())), "rbegin(initializer_list) is noexcept");
	static_assert(noexcept(eastl::rend(eastl::declval<std::initializer_list<int> >())), "rend(initializer_list) is noexcept");
}

EA_DISABLE_ALL_VC_WARNINGS()
#include <stdio.h>
#include <string.h>
EA_RESTORE_ALL_VC_WARNINGS()

template <class T>
using detect_iterator_traits_reference = typename eastl::iterator_traits<T>::reference;

// This is used below, though is currently disabled as documented below.
struct IListNode : public eastl::intrusive_list_node{};

int TestIterator_advance()
{
	int nErrorCount = 0;

	{
		// void advance(InputIterator& i, Distance n)
		const int num_elements = 10;
		int i;

		eastl::vector<int> v;
		for(i = 0; i < num_elements; i++)
			v.push_back(i);

		// test forward advancement
		eastl::vector<int>::iterator it = v.begin();
		for(i = 0; i < num_elements; i++)
		{
			EATEST_VERIFY(*it == v[i]);
			eastl::advance(it, 1);
		}

		// test backwards advancement
		eastl::vector<int>::iterator it2 = v.end();
		i = num_elements - 1;
		do
		{
			eastl::advance(it2, -1);
			EATEST_VERIFY(*it2 == v[i]);
		}
		while(i-- != 0);
	}

	{
		// void advance(InputIterator& i, Distance n)
		eastl::list<int> intList;
		intList.push_back(0);
		intList.push_back(1);
		intList.push_back(42);
		intList.push_back(2);

		eastl::list<int>::iterator it = intList.begin();
		eastl::advance(it, intList.size());
		EATEST_VERIFY(it == intList.end());

		// Exercise advance with an signed Distance type.
		it = intList.begin();
		eastl::advance(it, (eastl_ssize_t)intList.size());
		EATEST_VERIFY(it == intList.end());


		eastl::slist<int> intSlist;
		intSlist.push_front(0);
		intSlist.push_front(1);
		intSlist.push_front(42);
		intSlist.push_front(2);

		eastl::slist<int>::iterator its = intSlist.begin();
		eastl::advance(its, intSlist.size());
		EATEST_VERIFY(its == intSlist.end());

		// Exercise advance with an signed Distance type.
		its = intSlist.begin();
		eastl::advance(its, (eastl_ssize_t)intSlist.size());
		EATEST_VERIFY(its == intSlist.end());
	}

	{
		// void next(InputIterator& i, Distance n)
		eastl::vector<int> v;
		v.push_back(0);
		v.push_back(1);
		v.push_back(42);
		v.push_back(2);

		eastl::vector<int>::iterator it = v.begin();
		EATEST_VERIFY(*eastl::next(it, 0) == 0);
		EATEST_VERIFY(*eastl::next(it /*testing the iterator distance default value*/) == 1);
		EATEST_VERIFY(*eastl::next(it, 2) == 42);
	}

	{
		// void prev(InputIterator& i, Distance n)
		eastl::vector<int> v;
		v.push_back(0);
		v.push_back(1);
		v.push_back(42);
		v.push_back(2);

		eastl::vector<int>::iterator it = v.end();
		EATEST_VERIFY(*eastl::prev(it, 2) == 42);
		EATEST_VERIFY(*eastl::prev(it /*testing the iterator distance default value*/) == 2);
	}

	return nErrorCount;
}

int TestIterator_moveIterator()
{
	int nErrorCount = 0;

	{
		eastl::vector<int> v = {0, 1, 42, 2};
		const auto constBeginMoveIter = eastl::make_move_iterator(v.begin());

		// operator++(int)
		auto moveIter = constBeginMoveIter;
		moveIter++; // the result of the expression is the incremented value, we need this test to read the existing state of the iterator.
		EATEST_VERIFY(*moveIter != *constBeginMoveIter);

		// operator--(int)
		moveIter = constBeginMoveIter + 2; // points to '42'
		moveIter--; // the result of the expression is the incremented value, we need this test to read the existing state of the iterator.
		EATEST_VERIFY(*moveIter != *(constBeginMoveIter + 2));
	}

	{
		// Ensure that move_iterator indeed move yielded value whenever possible.
		auto x = eastl::make_unique<int>(42);
		auto* pX = &x;
		auto moveIter = eastl::make_move_iterator(pX);

		constexpr bool isCorrectReferenceType = eastl::is_same_v<decltype(moveIter)::reference, eastl::unique_ptr<int>&&>;
		constexpr bool isCorrectReturnType = eastl::is_same_v<decltype(*moveIter), eastl::unique_ptr<int>&&>;

		static_assert(isCorrectReferenceType, "move_iterator::reference has wrong type.");
		static_assert(isCorrectReturnType, "move_iterator::operator*() has wrong return type.");
		EATEST_VERIFY(isCorrectReferenceType);
		EATEST_VERIFY(isCorrectReturnType);

		auto pMoveX = *moveIter;
		EATEST_VERIFY(*pMoveX == 42);
	}

	// Bellow are regression tests that ensure we are covering the defect LWG 2106: http://cplusplus.github.io/LWG/lwg-defects.html#2106
	{
		// Check that we support iterators yielding const references.
		const int x = 42;
		const int* pX = &x;
		auto moveIter = eastl::make_move_iterator(pX);

		constexpr bool isCorrectReferenceType = eastl::is_same_v<decltype(moveIter)::reference, const int&&>;
		constexpr bool isCorrectReturnType = eastl::is_same_v<decltype(*moveIter), const int&&>;

		static_assert(isCorrectReferenceType, "move_iterator::reference has wrong type.");
		static_assert(isCorrectReturnType, "move_iterator::operator*() has wrong return type.");
		EATEST_VERIFY(isCorrectReferenceType);
		EATEST_VERIFY(isCorrectReturnType);

		auto pCopiedX = *moveIter;
		EATEST_VERIFY(pCopiedX == 42);
	}

	{
		// Check that we support iterators yielding plain value (typically a proxy-iterator).
		struct FakeProxyIterator
		{
			using iterator_category = eastl::forward_iterator_tag;
			using difference_type   = ptrdiff_t;
			using value_type        = int;
			using pointer           = int;  // Note that we are yielding by value.
			using reference         = int;  // Note that we are yielding by value.

			reference operator*() const { return 42; }
			pointer operator->() { return 42; }
			FakeProxyIterator& operator++() { return *this; }
			FakeProxyIterator operator++(int) { return {}; }

			bool operator==(const FakeProxyIterator&) { return true; };
			bool operator!=(const FakeProxyIterator&) { return false; };
		};

		FakeProxyIterator it = {};
		auto moveIter = eastl::make_move_iterator(it);

		constexpr bool isCorrectReferenceType = eastl::is_same_v<decltype(moveIter)::reference, int>;
		constexpr bool isCorrectReturnType = eastl::is_same_v<decltype(*moveIter), int>;

		static_assert(isCorrectReferenceType, "move_iterator::reference has wrong type.");
		static_assert(isCorrectReturnType, "move_iterator::operator*() has wrong return type.");
		EATEST_VERIFY(isCorrectReferenceType);
		EATEST_VERIFY(isCorrectReturnType);

		auto pCopiedX = *moveIter;
		EATEST_VERIFY(pCopiedX == 42);
	}

	{
		// test move_iterator usable in algorithms

		MoveOnlyType moveableArray[] = {MoveOnlyType{1}, MoveOnlyType{2}, MoveOnlyType{3}};
		MoveOnlyType destArray[] = {MoveOnlyType{0}, MoveOnlyType{0}, MoveOnlyType{0}};

		eastl::copy(eastl::make_move_iterator(moveableArray), eastl::make_move_iterator(moveableArray + 3), destArray);
		EATEST_VERIFY(VerifySequence(destArray, destArray + 3, {MoveOnlyType{1}, MoveOnlyType{2}, MoveOnlyType{3}},
		                             "copy(move_iterator(...))"));
	}

	return nErrorCount;
}


// An iterator over ints with its own iter_move, which counts its calls.
struct IterMoveCountingIterator
{
	using iterator_category = eastl::bidirectional_iterator_tag;
	using value_type        = int;
	using difference_type   = ptrdiff_t;
	using pointer           = int*;
	using reference         = int&;

	static int sIterMoveCount;
	int* mp;

	int& operator*() const { return *mp; }
	IterMoveCountingIterator& operator++() { ++mp; return *this; }
	IterMoveCountingIterator operator++(int) { IterMoveCountingIterator temp = *this; ++mp; return temp; }
	IterMoveCountingIterator& operator--() { --mp; return *this; }
	IterMoveCountingIterator operator--(int) { IterMoveCountingIterator temp = *this; --mp; return temp; }
	bool operator==(const IterMoveCountingIterator& x) const { return mp == x.mp; }
	bool operator!=(const IterMoveCountingIterator& x) const { return mp != x.mp; }

	friend int&& iter_move(const IterMoveCountingIterator& it) { ++sIterMoveCount; return eastl::move(*it.mp); }
};

int IterMoveCountingIterator::sIterMoveCount = 0;

int TestIterator_iterMove()
{
	int nErrorCount = 0;

	// The default moves from an lvalue and returns anything else as it is.
	{
		struct ProxyIterator { int operator*() const { return 42; } };

		static_assert(eastl::is_same_v<decltype(eastl::iter_move(eastl::declval<int* const&>())), int&&>, "iter_move(int*)");
		static_assert(eastl::is_same_v<decltype(eastl::iter_move(eastl::declval<const int* const&>())), const int&&>, "iter_move(const int*)");
		static_assert(eastl::is_same_v<decltype(eastl::iter_move(eastl::declval<const eastl::move_iterator<int*>&>())), int&&>, "iter_move(move_iterator<int*>)");
		static_assert(eastl::is_same_v<decltype(eastl::iter_move(eastl::declval<const ProxyIterator&>())), int>, "iter_move(ProxyIterator)");

		// noexcept when dereferencing is, like std's.
		struct NothrowIterator { int& operator*() const noexcept; };
		using eastl::iter_move;
		static_assert(noexcept(iter_move(eastl::declval<int* const&>())), "iter_move(int*) is noexcept");
		static_assert(noexcept(iter_move(eastl::declval<const NothrowIterator&>())), "iter_move(NothrowIterator) is noexcept");
		static_assert(!noexcept(iter_move(eastl::declval<const ProxyIterator&>())), "iter_move(ProxyIterator) isn't noexcept");
		static_assert(noexcept(iter_move(eastl::declval<const eastl::reverse_iterator<int*>&>())), "iter_move(reverse_iterator<int*>) is noexcept");
		static_assert(noexcept(*eastl::declval<const eastl::move_iterator<int*>&>()), "move_iterator<int*>::operator* is noexcept");

		eastl::vector<eastl::string> v = {"a", "b"};
		eastl::string s(eastl::iter_move(v.begin()));
		EATEST_VERIFY((s == "a") && v[0].empty());
	}

	// Algorithms and iterator adaptors move through an iterator's own iter_move, found by argument-dependent lookup.
	{
		using It = IterMoveCountingIterator;
		int a[4] = {1, 2, 3, 4};
		int b[4] = {};

		It::sIterMoveCount = 0;
		eastl::move(It{a}, It{a + 4}, b);
		EATEST_VERIFY((It::sIterMoveCount == 4) && (b[3] == 4));

		It::sIterMoveCount = 0;
		eastl::move_backward(It{a}, It{a + 4}, b + 4);
		EATEST_VERIFY(It::sIterMoveCount == 4);

		It::sIterMoveCount = 0;
		eastl::move(eastl::make_reverse_iterator(It{a + 4}), eastl::make_reverse_iterator(It{a}), b);
		EATEST_VERIFY((It::sIterMoveCount == 4) && (b[0] == 4) && (b[3] == 1));

		It::sIterMoveCount = 0;
		auto moveIter = eastl::make_move_iterator(It{a + 1});
		static_assert(eastl::is_same_v<decltype(*moveIter), int&&>, "move_iterator::operator*");
		EATEST_VERIFY((*moveIter == 2) && (It::sIterMoveCount == 1));

		It::sIterMoveCount = 0;
		It last = eastl::remove(It{a}, It{a + 4}, 2);
		EATEST_VERIFY((last.mp == a + 3) && (It::sIterMoveCount == 2) && (a[1] == 3) && (a[2] == 4));

		It::sIterMoveCount = 0;
		using eastl::iter_move;
		int i = iter_move(It{a});
		EATEST_VERIFY((i == 1) && (It::sIterMoveCount == 1));
	}

	return nErrorCount;
}



namespace IteratorGroupJ
{
	template <typename A, typename B> using equal_t = decltype(eastl::declval<A>() == eastl::declval<B>());
	template <typename A, typename B> using less_t = decltype(eastl::declval<A>() < eastl::declval<B>());
	template <typename A, typename B> using difference_t = decltype(eastl::declval<A>() - eastl::declval<B>());
	template <typename T> using arrow_t = decltype(eastl::declval<T>().operator->());
	template <typename T> using value_type_t = typename T::value_type;

	struct Unrelated {};

	// A forward iterator without a pointer member type.
	struct ForwardIterNoPointer
	{
		using difference_type = ptrdiff_t; using value_type = int; using reference = int&;
		using iterator_category = eastl::forward_iterator_tag;
		int& operator*() const;
	};

	// A random access iterator that returns by value and has no operator->.
	struct ProxyIterator
	{
		using difference_type = ptrdiff_t; using value_type = int; using pointer = void; using reference = int;
		using iterator_category = eastl::random_access_iterator_tag;
		int operator*() const;
		ProxyIterator& operator--();
	};

	// A bidirectional iterator with operator->, which reverse_iterator must call.
	struct ArrowIterator
	{
		using difference_type = ptrdiff_t; using value_type = int; using pointer = int*; using reference = int&;
		using iterator_category = eastl::bidirectional_iterator_tag;
		int* p;
		int& operator*() const { return *p; }
		int* operator->() const { return p; }
		ArrowIterator& operator--() { --p; return *this; }
	};
}

///////////////////////////////////////////////////////////////////////////////
// TestIterator
//
int TestIterator()
{
	int nErrorCount = 0;
	nErrorCount += TestIterator_advance();
	nErrorCount += TestIterator_moveIterator();
	nErrorCount += TestIterator_iterMove();

	{
		// reverse_iterator
		// reverse_iterator<Iterator> make_reverse_iterator(Iterator mi)
		{
			eastl::vector<int> src;
			for(int i = 0; i < 10; i++)
				src.push_back(i); // src should become {0,1,2,3,4,5,6,7,8,9}

			auto itr = eastl::make_reverse_iterator(src.end());
			EATEST_VERIFY(*itr == 9); ++itr;
			EATEST_VERIFY(*itr == 8); ++itr;
			EATEST_VERIFY(*itr == 7); ++itr;
			EATEST_VERIFY(*itr == 6); ++itr;
			EATEST_VERIFY(*itr == 5); ++itr;
			EATEST_VERIFY(*itr == 4); ++itr;
			EATEST_VERIFY(*itr == 3); ++itr;
			EATEST_VERIFY(*itr == 2); ++itr;
			EATEST_VERIFY(*itr == 1); ++itr;
			EATEST_VERIFY(*itr == 0); ++itr;
			EATEST_VERIFY( itr == src.rend());
			EATEST_VERIFY( itr == eastl::make_reverse_iterator(src.begin()));
		}
	}

	{
		// Regression bug with assign/insert combined with reverse iterator.
		eastl::vector<int> a;
		for (int i = 0; i < 10; ++i) {
			a.push_back(i);
		}

		eastl::deque<int> d;
		d.assign(a.rbegin(), a.rend());
		for (int i = 0; i < 10; ++i) {
			EATEST_VERIFY(a[i] == d[a.size() - i - 1]);
		}
		d.insert(d.end(), a.rbegin(), a.rend());
		for (int i = 0; i < 10; ++i) {
			EATEST_VERIFY(a[i] == d[d.size() - i - 1]);
		}

		eastl::vector<int> b;
		b.assign(a.rbegin(), a.rend());
		for (int i = 0; i < 10; ++i) {
			EATEST_VERIFY(a[i] == b[a.size() - i - 1]);
		}
		b.insert(b.end(), a.rbegin(), a.rend());
		for (int i = 0; i < 10; ++i) {
			EATEST_VERIFY(a[i] == b[b.size() - i - 1]);
		}
	}

	{
		// move_iterator
		// move_iterator<Iterator> make_move_iterator(Iterator mi)
		typedef eastl::vector<eastl::string> StringArray;

		StringArray src;
		for(eastl_size_t i = 0; i < 4; i++)
			src.push_back(eastl::string(1, (char8_t)('0' + i))); // v should become {"0", "1", "2", "3"};

		// Moves the values out of the string array and into the result.
		StringArray dst(eastl::make_move_iterator(src.begin()), eastl::make_move_iterator(src.end()));

		EATEST_VERIFY((src.size() == 4) && (src[0] ==  "") && (src[3] ==  ""));
		EATEST_VERIFY((dst.size() == 4) && (dst[0] == "0") && (dst[3] == "3"));
	}

	{
		// back_insert_iterator
		// back_inserter
		EA_CPP14_CONSTEXPR int n = 3;
		eastl::vector<TestObject> v1, v2, v3;
		v1.resize(n); v2.reserve(n); v3.reserve(n);
		{
			int64_t copyCtorCount0 = TestObject::sTOCopyCtorCount, moveCtorCount0 = TestObject::sTOMoveCtorCount;
			eastl::copy(v1.begin(), v1.end(), eastl::back_inserter(v2));
			EATEST_VERIFY(v1.size() == v2.size() && TestObject::sTOCopyCtorCount == (copyCtorCount0 + n) &&
				TestObject::sTOMoveCtorCount == moveCtorCount0);
		}
		{
			int64_t copyCtorCount0 = TestObject::sTOCopyCtorCount, moveCtorCount0 = TestObject::sTOMoveCtorCount;
			eastl::move(v1.begin(), v1.end(), eastl::back_inserter(v3));
			EATEST_VERIFY(v1.size() == v3.size() && TestObject::sTOCopyCtorCount == copyCtorCount0 &&
				TestObject::sTOMoveCtorCount == (moveCtorCount0 + n));
		}
	}

	{
		// front_insert_iterator
		// front_inserter
		// To do.
	}

	{
		// insert_iterator
		// inserter
		// To do.
	}

	{
		// difference_type distance(InputIterator first, InputIterator last)
		eastl::vector<int> intVector = {0, 1, 2, 3, 4, 5, 6, 7};
		EATEST_VERIFY(eastl::distance(intVector.begin(), intVector.end()) == 8);
    }


	{
		#if EASTL_BEGIN_END_ENABLED
			// begin / end
			// auto inline begin(Container& container) -> decltype(container.begin())
			// auto inline end(Container& container) -> decltype(container.end())

			eastl::vector<int> intVector;
			eastl::vector<int>::iterator intVectorIterator = eastl::begin(intVector);
			EATEST_VERIFY(intVectorIterator == eastl::end(intVector));

			eastl::list<int> intList;
			eastl::list<int>::iterator intListIterator = eastl::begin(intList);
			EATEST_VERIFY(intListIterator == eastl::end(intList));

			eastl::set<int> intSet;
			eastl::set<int>::iterator intSetIterator = eastl::begin(intSet);
			EATEST_VERIFY(intSetIterator == eastl::end(intSet));

			eastl::array<int, 0> intArray;
			eastl::array<int, 0>::iterator intArrayIterator = eastl::begin(intArray);
			EATEST_VERIFY(intArrayIterator == eastl::end(intArray));

			eastl::intrusive_list<IListNode> intIList;
			eastl::intrusive_list<IListNode>::iterator intIListIterator = eastl::begin(intIList);
			EATEST_VERIFY(intIListIterator == eastl::end(intIList));

			eastl::string8 str8;
			eastl::string8::iterator string8Iterator = eastl::begin(str8);
			EATEST_VERIFY(string8Iterator == eastl::end(str8));
		#endif
	}

	// eastl::data
	{
		eastl::array<int, 0> intArray;
		int* pIntArrayData = eastl::data(intArray);
		EATEST_VERIFY(pIntArrayData == intArray.data());

		eastl::vector<int> intVector;
		int* pIntVectorData = eastl::data(intVector);
		EATEST_VERIFY(pIntVectorData == intVector.data());

		int intCArray[34];
		int* pIntCArray = eastl::data(intCArray);
		EATEST_VERIFY(pIntCArray == intCArray);

		std::initializer_list<int> intInitList;
		const int* pIntInitList = eastl::data(intInitList);
		EATEST_VERIFY(pIntInitList == intInitList.begin());
	}

	// eastl::size
	{
		eastl::vector<int> intVector;
		intVector.push_back();
		intVector.push_back();
		intVector.push_back();
		EATEST_VERIFY(eastl::size(intVector) == 3);

		int intCArray[34];
		EATEST_VERIFY(eastl::size(intCArray) == 34);
		static_assert(eastl::size(intCArray) == 34, "eastl::size failure");
	}

	// eastl::ssize
	{
		eastl::vector<int> intVector;
		intVector.push_back();
		intVector.push_back();
		intVector.push_back();
		EATEST_VERIFY(eastl::ssize(intVector) == (signed)3);

		int intCArray[34];
		EATEST_VERIFY(eastl::ssize(intCArray) == (signed)34);
		static_assert(eastl::ssize(intCArray) == 34, "eastl::ssize failure");
	}

	// eastl::empty
	{
		eastl::vector<int> intVector;
		EATEST_VERIFY(eastl::empty(intVector));
		intVector.push_back();
		EATEST_VERIFY(!eastl::empty(intVector));

		std::initializer_list<int> intInitListEmpty;
		EATEST_VERIFY(eastl::empty(intInitListEmpty));
		EATEST_VERIFY(!eastl::empty({1, 2, 3, 4, 5, 6}));
	}

	// Range-based for loops
	{
		{
			eastl::vector<int> v;
			int I = 0;

			v.push_back(0);
			v.push_back(1);

			for(int i : v)
				EATEST_VERIFY(i == I++);
		}

		{
			eastl::string s8;
			char C = 'a';

			s8.push_back('a');
			s8.push_back('b');

			for(char c : s8)
				EATEST_VERIFY(c == C++);
		}
	}

	EASTL_INTERNAL_DISABLE_DEPRECATED() // '*': was declared deprecated
	{
		// is_iterator_wrapper
		static_assert((eastl::is_iterator_wrapper<void>::value													== false),  "is_iterator_wrapper failure");
		static_assert((eastl::is_iterator_wrapper<int>::value													== false),  "is_iterator_wrapper failure");
		static_assert((eastl::is_iterator_wrapper<int*>::value													== false),  "is_iterator_wrapper failure");
		static_assert((eastl::is_iterator_wrapper<eastl::array<int, 2>::iterator>::value							== false),   "is_iterator_wrapper failure");
		static_assert((eastl::is_iterator_wrapper<eastl::array<char, 2>*>::value									== false),  "is_iterator_wrapper failure");
		static_assert((eastl::is_iterator_wrapper<eastl::vector<char> >::value									== false),  "is_iterator_wrapper failure");
		static_assert((eastl::is_iterator_wrapper<eastl::generic_iterator<int*> >::value						== true),   "is_iterator_wrapper failure");
		static_assert((eastl::is_iterator_wrapper<eastl::move_iterator<eastl::array<int, 2>::iterator> >::value	== true),   "is_iterator_wrapper failure");
		static_assert((eastl::is_iterator_wrapper<eastl::reverse_iterator<eastl::array<int, 2>::iterator> >::value == false),  "is_iterator_wrapper failure");
		static_assert((eastl::is_iterator_wrapper<eastl::reverse_iterator<int*> >::value						== false),  "is_iterator_wrapper failure");
		static_assert((eastl::is_iterator_wrapper<eastl::reverse_iterator<eastl::move_iterator<int*>> >::value  == true),   "is_iterator_wrapper failure");
	}


	{
		// unwrap_iterator
		int intArray[2];
		int* pInt = eastl::unwrap_iterator(&intArray[0]);
		intArray[0] = 17;
		EATEST_VERIFY(*pInt == 17);
		static_assert((eastl::is_same<decltype(eastl::unwrap_iterator(&intArray[0])), int*>::value == true),  "unwrap_iterator failure");

		eastl::generic_iterator<int*> giIntArray(intArray);
		pInt = eastl::unwrap_iterator(giIntArray);
		intArray[0] = 18;
		EATEST_VERIFY(*pInt == 18);
		static_assert((eastl::is_same<decltype(eastl::unwrap_iterator(giIntArray)), int*>::value == true),  "unwrap_iterator failure");

		eastl::vector<int> intVector(4, 19);
		eastl::vector<int>::iterator itVector = eastl::unwrap_iterator(intVector.begin());
		EATEST_VERIFY(*itVector == 19);
		static_assert((eastl::is_same<decltype(eastl::unwrap_iterator(intVector.begin())), eastl::vector<int>::iterator>::value == true),  "unwrap_iterator failure");

		eastl::move_iterator<eastl::vector<int>::iterator> miIntVector(intVector.begin());
		itVector = eastl::unwrap_iterator(miIntVector);
		intVector[0] = 20;
		EATEST_VERIFY(*itVector == 20);
		static_assert((eastl::is_same<decltype(eastl::unwrap_iterator(miIntVector)), eastl::vector<int>::iterator>::value == true),  "unwrap_iterator failure");

		eastl::reverse_iterator<eastl::vector<int>::iterator> riIntVector = intVector.rbegin();
		eastl::reverse_iterator<eastl::vector<int>::iterator> riUnwrapped = eastl::unwrap_iterator(riIntVector);
		EATEST_VERIFY(*riUnwrapped == 19);
		static_assert((eastl::is_same<decltype(eastl::unwrap_iterator(riIntVector)), eastl::reverse_iterator<eastl::vector<int>::iterator>>::value == true), "unwrap_iterator failure");

		eastl::reverse_iterator<eastl::move_iterator<eastl::vector<int>::iterator>> rimiIntVec(miIntVector);
		static_assert((eastl::is_same<decltype(eastl::unwrap_iterator(rimiIntVec)), eastl::reverse_iterator<eastl::vector<int>::iterator>>::value == true), "unwrap_iterator failure");

		eastl::reverse_iterator<eastl::generic_iterator<int*>> rigiIntArray(giIntArray);
		static_assert((eastl::is_same<decltype(eastl::unwrap_iterator(rigiIntArray)), eastl::reverse_iterator<int*>>::value == true), "unwrap_iterator failure");

		eastl::deque<int> intDeque(3);
		eastl::deque<int>::iterator begin = intDeque.begin();
		eastl::generic_iterator<eastl::deque<int>::iterator> giWrappedBegin(begin);
		static_assert((eastl::is_same<decltype(eastl::unwrap_iterator(giWrappedBegin)), eastl::deque<int>::iterator>::value == true), "unwrap_iterator failure");

		eastl::deque<int>::iterator unwrappedBegin = eastl::unwrap_iterator(giWrappedBegin);
		EATEST_VERIFY(begin == unwrappedBegin);
	}

	{
		// unwrap_generic_iterator
		int intArray[2] = {0, 1};
		eastl::generic_iterator<int*> giIntArray(intArray);
		int* pInt = eastl::unwrap_generic_iterator(giIntArray);
		EATEST_VERIFY(*pInt == 0);
		static_assert((eastl::is_same<decltype(eastl::unwrap_generic_iterator(giIntArray)), int*>::value == true),  "unwrap_iterator failure");

		eastl::move_iterator<int*> miIntArray(intArray);
		static_assert((eastl::is_same<decltype(eastl::unwrap_generic_iterator(miIntArray)), eastl::move_iterator<int*>>::value == true),  "unwrap_iterator failure");

		eastl::vector<int> intVector(1, 1);
		eastl::generic_iterator<eastl::vector<int>::iterator> giVectorInt(intVector.begin());
		eastl::vector<int>::iterator it = unwrap_generic_iterator(giVectorInt);
		EATEST_VERIFY(*it == 1);
		static_assert((eastl::is_same<decltype(eastl::unwrap_generic_iterator(giVectorInt)), eastl::vector<int>::iterator>::value == true),  "unwrap_iterator failure");
	}

	{
		// unwrap_move_iterator
		int intArray[2] = {0, 1};
		eastl::move_iterator<int*> miIntArray(intArray);
		int* pInt = eastl::unwrap_move_iterator(miIntArray);
		EATEST_VERIFY(*pInt == 0);
		static_assert((eastl::is_same<decltype(eastl::unwrap_move_iterator(miIntArray)), int*>::value == true),  "unwrap_iterator failure");

		eastl::generic_iterator<int*> giIntArray(intArray);
		static_assert((eastl::is_same<decltype(eastl::unwrap_move_iterator(giIntArray)), eastl::generic_iterator<int*>>::value == true),  "unwrap_iterator failure");

		eastl::vector<int> intVector(1, 1);
		eastl::move_iterator<eastl::vector<int>::iterator> miVectorInt(intVector.begin());
		eastl::vector<int>::iterator it = unwrap_move_iterator(miVectorInt);
		EATEST_VERIFY(*it == 1);
		static_assert((eastl::is_same<decltype(eastl::unwrap_move_iterator(miVectorInt)), eastl::vector<int>::iterator>::value == true),  "unwrap_iterator failure");
    }
    EASTL_INTERNAL_RESTORE_DEPRECATED()

	{
		// array cbegin - cend
		int arr[3]{ 1, 2, 3 };
		auto b = eastl::cbegin(arr);
		auto e = eastl::cend(arr);
		EATEST_VERIFY(*b == 1);

		auto dist = eastl::distance(b,e);
		EATEST_VERIFY(dist == 3);
	}

	{
		// Regression test that ensure N3844 is working correctly.
		static_assert(!eastl::is_detected<detect_iterator_traits_reference, int>::value, "detecting iterator_traits<int> should SFINAE gracefully.");
	}

	// The C++20 iterator_traits, reverse_iterator, move_iterator and move_sentinel rules, like std.
	{
		using namespace IteratorGroupJ;

		static_assert(!eastl::is_detected<value_type_t, eastl::iterator_traits<void*>>::value && !eastl::is_detected<value_type_t, eastl::iterator_traits<const void*>>::value, "iterator_traits<void*> is empty");
		static_assert(!eastl::is_detected<value_type_t, eastl::iterator_traits<void (*)()>>::value, "iterator_traits of a function pointer is empty");
		static_assert(eastl::is_same<eastl::iterator_traits<volatile int*>::value_type, int>::value && eastl::is_same<eastl::iterator_traits<const int*>::reference, const int&>::value, "iterator_traits of a cv pointer");
		static_assert(eastl::is_same<eastl::iterator_traits<ForwardIterNoPointer>::pointer, void>::value, "iterator_traits without a pointer member type");
	#if !EASTL_STD_ITERATOR_CATEGORY_ENABLED || defined(EA_COMPILER_CPP20_ENABLED)
		static_assert(eastl::is_same<eastl::iterator_traits<int*>::iterator_concept, eastl::contiguous_iterator_tag>::value, "iterator_traits<int*>::iterator_concept");
	#endif

		typedef eastl::reverse_iterator<int*> RI;
		typedef eastl::reverse_iterator<const int*> CRI;
		static_assert(!eastl::is_constructible<RI, CRI>::value && !eastl::is_assignable<RI&, CRI>::value && eastl::is_convertible<RI, CRI>::value, "reverse_iterator conversions");
		static_assert(!eastl::is_detected<equal_t, RI, eastl::reverse_iterator<Unrelated*>>::value && !eastl::is_detected<less_t, RI, eastl::reverse_iterator<Unrelated*>>::value &&
		              !eastl::is_detected<difference_t, RI, eastl::reverse_iterator<Unrelated*>>::value, "reverse_iterator comparisons of incomparable iterators");
		static_assert(eastl::is_same<difference_t<RI, CRI>, ptrdiff_t>::value, "reverse_iterator difference");
		static_assert(!eastl::is_detected<arrow_t, const eastl::reverse_iterator<ProxyIterator>&>::value, "reverse_iterator::operator-> needs a pointer or a base operator->");
		static_assert(eastl::is_same<RI::iterator_concept, eastl::random_access_iterator_tag>::value && eastl::is_same<eastl::reverse_iterator<ArrowIterator>::iterator_concept, eastl::bidirectional_iterator_tag>::value, "reverse_iterator::iterator_concept");

		typedef eastl::move_iterator<int*> MI;
		typedef eastl::move_iterator<const int*> CMI;
		static_assert(!eastl::is_constructible<MI, CMI>::value && !eastl::is_assignable<MI&, CMI>::value && eastl::is_convertible<MI, CMI>::value, "move_iterator conversions");
		static_assert(!eastl::is_detected<equal_t, MI, eastl::move_iterator<Unrelated*>>::value && !eastl::is_detected<less_t, MI, eastl::move_iterator<Unrelated*>>::value, "move_iterator comparisons of incomparable iterators");
		static_assert(eastl::is_same<MI::iterator_concept, eastl::random_access_iterator_tag>::value && eastl::is_same<eastl::move_iterator<ForwardIterNoPointer>::iterator_concept, eastl::forward_iterator_tag>::value, "move_iterator::iterator_concept");

		int a[3] = { 1, 2, 3 };

		RI ri(a + 3);
		CRI cri(ri);
		cri = RI(a + 2);
		EATEST_VERIFY((cri.base() == a + 2) && (ri != cri) && (ri < cri) && (cri - ri == 1) && (*ri.operator->() == 3));
		eastl::reverse_iterator<ArrowIterator> rai(ArrowIterator{a + 2});
		EATEST_VERIFY(rai.operator->() == a + 1);

		MI mi(a);
		CMI cmi(mi);
		cmi = MI(a + 1); // The converting assignment, which didn't compile before (it read a protected member of another specialization).
		EATEST_VERIFY((cmi.base() == a + 1) && (cmi != mi) && (mi < cmi));

		eastl::move_sentinel<const int*> sentinel(a + 3);
		int sum = 0;
		for (CMI it(a); it != sentinel; ++it)
			sum += *it;
		EATEST_VERIFY((sum == 6) && (sentinel - cmi == 2) && (cmi - sentinel == -2) && (CMI(a + 3) == sentinel));

	#if defined(EA_COMPILER_HAS_THREE_WAY_COMPARISON)
		EATEST_VERIFY(((ri <=> cri) < 0) && ((mi <=> cmi) < 0));
	#endif
	}

	return nErrorCount;
}
