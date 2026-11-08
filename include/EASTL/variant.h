///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// Implements the class template variant represents a type-safe union. An
// instance of variant at any given time either holds a value of one of its
// alternative types, or it holds no value.
//
// As with unions, if a variant holds a value of some object type T, the object
// representation of T is allocated directly within the object representation of
// the variant itself.
//
// Variant is not allowed to allocate additional (dynamic) memory.
//
// A variant is not permitted to hold references, arrays, or the type void.
// Empty variants are also ill-formed (variant<monostate> can be used instead).
//
// A variant is permitted to hold the same type more than once, and to hold
// differently cv-qualified versions of the same type.  As with unions, the
// default-initialized variant holds a value of its first alternative, unless
// that alternative is not default-constructible (in which case default
// constructor won't compile: the helper class monostate can be used to make
// such variants default-constructible)
//
// Given defect 2901, the eastl::variant implementation does not provide the
// specified allocator-aware functions.  This will be re-evaluated when the LWG
// addresses this issue in future standardization updates.
// LWG Defect 2901: https://cplusplus.github.io/LWG/issue2901
//
// Allocator-extended constructors
// template <class Alloc> variant(allocator_arg_t, const Alloc&);
// template <class Alloc> variant(allocator_arg_t, const Alloc&, const variant&);
// template <class Alloc> variant(allocator_arg_t, const Alloc&, variant&&);
// template <class Alloc, class T> variant(allocator_arg_t, const Alloc&, T&&);
// template <class Alloc, class T, class... Args> variant(allocator_arg_t, const Alloc&, in_place_type_t<T>, Args&&...);
// template <class Alloc, class T, class U, class... Args> variant(allocator_arg_t, const Alloc&, in_place_type_t<T>, initializer_list<U>, Args&&...);
// template <class Alloc, size_t I, class... Args> variant(allocator_arg_t, const Alloc&, in_place_index_t<I>, Args&&...);
// template <class Alloc, size_t I, class U, class... Args> variant(allocator_arg_t, const Alloc&, in_place_index_t<I>, initializer_list<U>, Args&&...);
//
// 20.7.12, allocator-related traits
// template <class T, class Alloc>        struct uses_allocator;
// template <class... Types, class Alloc> struct uses_allocator<variant<Types...>, Alloc>;
//
// eastl::variant doesn't support:
//  * recursive variant support
//  * strong exception guarantees as specified (we punted on the assignment problem).
//    if an exception is thrown during assignment its undefined behaviour in our implementation.
//
// Reference:
// 	* http://en.cppreference.com/w/cpp/utility/variant
// 	* https://thenewcpp.wordpress.com/2012/02/15/variadic-templates-part-3-or-how-i-wrote-a-variant-class/
///////////////////////////////////////////////////////////////////////////

#ifndef EASTL_VARIANT_H
#define EASTL_VARIANT_H

#include <EASTL/internal/config.h>
#include <EASTL/internal/in_place_t.h>
#include <EASTL/internal/integer_sequence.h>
#include <EASTL/internal/special_member_functions_variant_optional.h>
#include <EASTL/meta.h>
#include <EASTL/utility.h>
#include <EASTL/functional.h>
#include <EASTL/initializer_list.h>
#include <EASTL/tuple.h>
#include <EASTL/type_traits.h>
#include <EASTL/array.h>

#if defined(EA_COMPILER_CPP20_ENABLED)
	#include <memory> // std::construct_at, to begin the lifetime of an alternative in a constant expression
#endif

#if EASTL_EXCEPTIONS_ENABLED
	#include <stdexcept>
	#include <exception>
#endif


#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif

#ifndef EA_COMPILER_CPP14_ENABLED
	static_assert(false, "eastl::variant requires a C++14 compatible compiler (at least) ");
#endif

EA_DISABLE_VC_WARNING(4625) // copy constructor was implicitly defined as deleted

namespace eastl
{
	///////////////////////////////////////////////////////////////////////////
	// 20.7.3, variant_npos
	//
	EASTL_CPP17_INLINE_VARIABLE EA_CONSTEXPR size_t variant_npos = size_t(-1);


	///////////////////////////////////////////////////////////////////////////
	// 20.7.10, class bad_variant_access
	//
	#if EASTL_EXCEPTIONS_ENABLED
		struct bad_variant_access : public std::logic_error
		{
			bad_variant_access() : std::logic_error("eastl::bad_variant_access exception") {}
			virtual ~bad_variant_access() EA_NOEXCEPT {}
		};
	#endif


	///////////////////////////////////////////////////////////////////////////
	// 20.7.7, class monostate
	//
	// Unit type intended for use as a well-behaved empty alternative in
	// variant. A variant of non-default-constructible types may list monostate
	// as its first alternative: this makes the variant itself default-contructible.
	//
	struct monostate {};

	// 20.7.8, monostate relational operators
#if defined(EA_COMPILER_HAS_THREE_WAY_COMPARISON)
	EA_CONSTEXPR std::strong_ordering operator<=>(monostate, monostate) EA_NOEXCEPT { return std::strong_ordering::equal; }
#else
	EA_CONSTEXPR bool operator> (monostate, monostate) EA_NOEXCEPT { return false; }
	EA_CONSTEXPR bool operator< (monostate, monostate) EA_NOEXCEPT { return false; }
	EA_CONSTEXPR bool operator!=(monostate, monostate) EA_NOEXCEPT { return false; }
	EA_CONSTEXPR bool operator<=(monostate, monostate) EA_NOEXCEPT { return true; }
	EA_CONSTEXPR bool operator>=(monostate, monostate) EA_NOEXCEPT { return true; }
#endif
	EA_CONSTEXPR bool operator==(monostate, monostate) EA_NOEXCEPT { return true; }

	// 20.7.11, hash support
	template <class T> struct hash;
	template <> struct hash<monostate>
		{ size_t operator()(monostate) const { return static_cast<size_t>(-0x42); } };


	///////////////////////////////////////////////////////////////////////////
	// 20.7.2, forward-declaration for types that depend on the variant
	//
	template <class... Types>
	class variant;


	///////////////////////////////////////////////////////////////////////////
	// 20.7.3, variant_size, variant_size_v helper classes
	//
	template <class T>        struct variant_size;
	template <class T>        struct variant_size<const T>           : integral_constant<size_t, variant_size<T>::value>  {};
	template <class T>        struct variant_size<volatile T>        : integral_constant<size_t, variant_size<T>::value>  {};
	template <class T>        struct variant_size<const volatile T>  : integral_constant<size_t, variant_size<T>::value>  {};
	template <class... Types> struct variant_size<variant<Types...>> : integral_constant<size_t, sizeof...(Types)> {};

	// variant_size_v template alias
	template <typename T>
	EASTL_CPP17_INLINE_VARIABLE EA_CONSTEXPR size_t variant_size_v = variant_size<T>::value;


	///////////////////////////////////////////////////////////////////////////
	// variant_alternative_helper
	//
	// This helper does the heavy lifting of traversing the variadic type list
	// and retrieving the type at the user provided index.
	//
	template <size_t I, typename... Ts>
	struct variant_alternative_helper;

	template <size_t I, typename Head, typename... Tail>
	struct variant_alternative_helper<I, Head, Tail...>
		{ typedef typename variant_alternative_helper<I - 1, Tail...>::type type; };

	template <typename Head, typename... Tail>
	struct variant_alternative_helper<0, Head, Tail...>
		{ typedef Head type; };


	///////////////////////////////////////////////////////////////////////////
	// 20.7.4, variant_alternative
	//
	template <size_t I, class T> struct variant_alternative;
	template <size_t I, class... Types> struct variant_alternative<I, variant<Types...>> : variant_alternative_helper<I, Types...> {};

	// ISO required cv-qualifer specializations
	template <size_t I, class T> struct variant_alternative<I, const T>          : add_const<typename variant_alternative<I, T>::type> {};
	template <size_t I, class T> struct variant_alternative<I, volatile T>       : add_volatile<typename variant_alternative<I, T>::type> {};
	template <size_t I, class T> struct variant_alternative<I, const volatile T> : add_cv<typename variant_alternative<I, T>::type> {};

	// variant_alternative_t template alias
	template <size_t I, class T> using variant_alternative_t = typename variant_alternative<I, T>::type;

	///////////////////////////////////////////////////////////////////////////
	// variant storage, compile-time alternative access and visitation
	//
	// The alternatives live in a recursive union (the first alternative, or a union of the remaining ones), like MSVC
	// STL, libc++ and libstdc++. Alternative I is reached through I member accesses resolved at compile time, all at
	// offset 0, so get, visit and the special member functions never search the sub-objects at runtime, and the
	// storage is usable in constant expressions.
	//
	// Visitation follows MSVC STL: the variants' indices form one combined state, and a switch on it (a jump table
	// or a few compares once optimized) calls the visitor with the alternatives of that state, each case inlined.
	// Only more than 256 combined states fall back to a table of function pointers.
	//
	namespace internal
	{
		// The smallest signed type that holds every alternative index, with variant_npos stored as -1 (MSVC STL's
		// encoding; libstdc++ uses the same one for up to 127 alternatives). index() is then a plain sign extension.
		template <size_t Count>
		using variant_index_t = conditional_t<(Count < 127), signed char, conditional_t<(Count < 32767), short, int>>;

		// The number of valueless states visitation has to handle. EASTL only leaves a variant valueless when an
		// exception interrupts an assignment or emplace, so without exceptions there is none.
		EASTL_CPP17_INLINE_VARIABLE EA_CONSTEXPR size_t variant_valueless_states = EASTL_EXCEPTIONS_ENABLED ? 1 : 0;

		template <bool TriviallyDestructible, class... Types>
		class variant_storage_impl
		{
			// the empty end of the recursion
		};

		// (fold expressions rather than recursive traits keep the instantiation depth of large variants low)
		template <class... Types>
		using variant_storage = variant_storage_impl<(is_trivially_destructible_v<Types> && ...), Types...>;

		EA_DISABLE_VC_WARNING(4702) // unreachable code: the code after constructing an alternative whose constructor always throws

		template <class First, class... Rest>
		class variant_storage_impl<true, First, Rest...>
		{
		public:
			static EA_CONSTEXPR size_t size = 1 + sizeof...(Rest);

			union
			{
				remove_cv_t<First> mHead;
				variant_storage<Rest...> mTail;
			};

			EA_CPP20_CONSTEXPR variant_storage_impl() EA_NOEXCEPT {} // no active alternative

			template <class... Args>
			EA_CONSTEXPR explicit variant_storage_impl(integral_constant<size_t, 0>, Args&&... args)
				: mHead(eastl::forward<Args>(args)...)
			{
			}

			template <size_t I, class... Args, enable_if_t<(I > 0), int> = 0>
			EA_CONSTEXPR explicit variant_storage_impl(integral_constant<size_t, I>, Args&&... args)
				: mTail(integral_constant<size_t, I - 1>{}, eastl::forward<Args>(args)...)
			{
			}

			variant_storage_impl(const variant_storage_impl&) = default;
			variant_storage_impl(variant_storage_impl&&) = default;
			variant_storage_impl& operator=(const variant_storage_impl&) = default;
			variant_storage_impl& operator=(variant_storage_impl&&) = default;

			EA_CONSTEXPR First& get() & EA_NOEXCEPT { return mHead; }
			EA_CONSTEXPR const First& get() const& EA_NOEXCEPT { return mHead; }
			EA_CONSTEXPR First&& get() && EA_NOEXCEPT { return eastl::move(mHead); }
			EA_CONSTEXPR const First&& get() const&& EA_NOEXCEPT { return eastl::move(mHead); }
		};

		template <class First, class... Rest>
		class variant_storage_impl<false, First, Rest...>
		{
		public:
			static EA_CONSTEXPR size_t size = 1 + sizeof...(Rest);

			union
			{
				remove_cv_t<First> mHead;
				variant_storage<Rest...> mTail;
			};

			EA_CPP20_CONSTEXPR variant_storage_impl() EA_NOEXCEPT {} // no active alternative

			template <class... Args>
			EA_CONSTEXPR explicit variant_storage_impl(integral_constant<size_t, 0>, Args&&... args)
				: mHead(eastl::forward<Args>(args)...)
			{
			}

			template <size_t I, class... Args, enable_if_t<(I > 0), int> = 0>
			EA_CONSTEXPR explicit variant_storage_impl(integral_constant<size_t, I>, Args&&... args)
				: mTail(integral_constant<size_t, I - 1>{}, eastl::forward<Args>(args)...)
			{
			}

			// The variant destroys the active alternative; this destructor only exists because the union has a member
			// with a non-trivial destructor.
			EA_CPP20_CONSTEXPR ~variant_storage_impl() {}

			variant_storage_impl(const variant_storage_impl&) = default;
			variant_storage_impl(variant_storage_impl&&) = default;
			variant_storage_impl& operator=(const variant_storage_impl&) = default;
			variant_storage_impl& operator=(variant_storage_impl&&) = default;

			EA_CONSTEXPR First& get() & EA_NOEXCEPT { return mHead; }
			EA_CONSTEXPR const First& get() const& EA_NOEXCEPT { return mHead; }
			EA_CONSTEXPR First&& get() && EA_NOEXCEPT { return eastl::move(mHead); }
			EA_CONSTEXPR const First&& get() const&& EA_NOEXCEPT { return eastl::move(mHead); }
		};

		EA_RESTORE_VC_WARNING() // 4702

		// Alternative I of a variant_storage, with the storage's value category.
		template <size_t I, class Storage>
		EA_CONSTEXPR decltype(auto) variant_raw_get(Storage&& storage) EA_NOEXCEPT
		{
			if constexpr (I == 0)
				return static_cast<Storage&&>(storage).get();
			else if constexpr (I == 1)
				return static_cast<Storage&&>(storage).mTail.get();
			else if constexpr (I == 2)
				return static_cast<Storage&&>(storage).mTail.mTail.get();
			else if constexpr (I == 3)
				return static_cast<Storage&&>(storage).mTail.mTail.mTail.get();
			else if constexpr (I == 4)
				return static_cast<Storage&&>(storage).mTail.mTail.mTail.mTail.get();
			else if constexpr (I == 5)
				return static_cast<Storage&&>(storage).mTail.mTail.mTail.mTail.mTail.get();
			else if constexpr (I == 6)
				return static_cast<Storage&&>(storage).mTail.mTail.mTail.mTail.mTail.mTail.get();
			else if constexpr (I == 7)
				return static_cast<Storage&&>(storage).mTail.mTail.mTail.mTail.mTail.mTail.mTail.get();
			else
				return internal::variant_raw_get<I - 8>(
					static_cast<Storage&&>(storage).mTail.mTail.mTail.mTail.mTail.mTail.mTail.mTail);
		}

		// Begins the lifetime of storage (which has no active alternative) with the given constructor arguments.
		template <class Storage, class... Args>
		EA_CPP20_CONSTEXPR void variant_construct_storage(Storage& storage, Args&&... args)
		{
		#if defined(EA_COMPILER_CPP20_ENABLED)
			if (eastl::is_constant_evaluated())
			{
				std::construct_at(&storage, eastl::forward<Args>(args)...);
				return;
			}
		#endif
			::new (static_cast<void*>(&storage)) Storage(eastl::forward<Args>(args)...);
		}

		// A reference to an alternative (or to the storage for the valueless state, I == variant_npos) with its index.
		EA_DISABLE_VC_WARNING(4623) // default constructor was implicitly defined as deleted: the reference member has no default.
		template <class T, size_t I>
		struct variant_tagged
		{
			static EA_CONSTEXPR size_t index = I;
			T value;
		};
		EA_RESTORE_VC_WARNING()

		template <class Storage, size_t I>
		using variant_tagged_ref_t = variant_tagged<decltype(internal::variant_raw_get<I>(eastl::declval<Storage>())), I>;

		// Grants the free functions access to the storage of a variant (a private base).
		struct variant_access
		{
			template <class Variant>
			static EA_CONSTEXPR auto&& storage(Variant&& v) EA_NOEXCEPT
			{
				return eastl::forward<Variant>(v).mStorage;
			}
		};

		EA_CONSTEXPR int variant_dispatch_strategy(size_t states)
		{
			return states <= 4 ? 1 : states <= 16 ? 2 : states <= 64 ? 3 : states <= 256 ? 4 : -1;
		}

		template <class Dispatcher, class States = make_index_sequence<Dispatcher::states>>
		struct variant_dispatch_table;

		template <class Dispatcher, size_t... States>
		struct variant_dispatch_table<Dispatcher, index_sequence<States...>>
		{
			static EA_CONSTEXPR decltype(&Dispatcher::template call<0>) table[] = {&Dispatcher::template call<States>...};
		};

		// variant_dispatch<Strategy>::call<Dispatcher>(state, args...) calls Dispatcher::call<State>(args...) for the
		// runtime state in [0, Dispatcher::states), where State is that state as a constant.
		template <int Strategy>
		struct variant_dispatch
		{
			// More than 256 states: a table of function pointers.
			template <class Dispatcher, class... Args>
			static EA_CONSTEXPR typename Dispatcher::result_type call(size_t state, Args&&... args)
			{
				return variant_dispatch_table<Dispatcher>::table[state](eastl::forward<Args>(args)...);
			}
		};

	#if defined(_MSC_VER) && !defined(__clang__)
		#define EASTL_VARIANT_UNREACHABLE() __assume(0)
	#else
		#define EASTL_VARIANT_UNREACHABLE() __builtin_unreachable()
	#endif

	#define EASTL_VARIANT_STAMP4(n, x) x(n); x(n + 1); x(n + 2); x(n + 3)
	#define EASTL_VARIANT_STAMP16(n, x) \
		EASTL_VARIANT_STAMP4(n, x); EASTL_VARIANT_STAMP4(n + 4, x); EASTL_VARIANT_STAMP4(n + 8, x); EASTL_VARIANT_STAMP4(n + 12, x)
	#define EASTL_VARIANT_STAMP64(n, x) \
		EASTL_VARIANT_STAMP16(n, x); EASTL_VARIANT_STAMP16(n + 16, x); EASTL_VARIANT_STAMP16(n + 32, x); EASTL_VARIANT_STAMP16(n + 48, x)
	#define EASTL_VARIANT_STAMP256(n, x) \
		EASTL_VARIANT_STAMP64(n, x); EASTL_VARIANT_STAMP64(n + 64, x); EASTL_VARIANT_STAMP64(n + 128, x); EASTL_VARIANT_STAMP64(n + 192, x)

	#define EASTL_VARIANT_CASE(n)                                                   \
		case (n):                                                                   \
			if constexpr ((n) < Dispatcher::states)                                 \
				return Dispatcher::template call<(n)>(eastl::forward<Args>(args)...); \
			else                                                                    \
				EASTL_VARIANT_UNREACHABLE();                                        \
			[[fallthrough]]

	#define EASTL_VARIANT_DISPATCH(strategy, stamp)                                                                \
		template <>                                                                                      \
		struct variant_dispatch<strategy>                                                                 \
		{                                                                                                \
			template <class Dispatcher, class... Args>                                                   \
			static EA_CONSTEXPR typename Dispatcher::result_type call(size_t state, Args&&... args)      \
			{                                                                                            \
				switch (state)                                                                           \
				{                                                                                        \
					EASTL_VARIANT_STAMP##stamp(0, EASTL_VARIANT_CASE);                                   \
				default:                                                                                 \
					EASTL_VARIANT_UNREACHABLE();                                                         \
				}                                                                                        \
			}                                                                                            \
		}

		EASTL_VARIANT_DISPATCH(1, 4);
		EASTL_VARIANT_DISPATCH(2, 16);
		EASTL_VARIANT_DISPATCH(3, 64);
		EASTL_VARIANT_DISPATCH(4, 256);

	#undef EASTL_VARIANT_DISPATCH
	#undef EASTL_VARIANT_CASE
	#undef EASTL_VARIANT_STAMP256
	#undef EASTL_VARIANT_STAMP64
	#undef EASTL_VARIANT_STAMP16
	#undef EASTL_VARIANT_STAMP4
	#undef EASTL_VARIANT_UNREACHABLE

		// Internal visitation of one storage: Fn is called with variant_tagged<reference, I>, including the valueless
		// state (I == variant_npos, the reference is to the storage) when exceptions are enabled.
		template <class Fn, class Storage>
		struct variant_raw_visit_dispatcher
		{
			static EA_CONSTEXPR size_t states = remove_reference_t<Storage>::size + variant_valueless_states;
			using result_type = decltype(eastl::declval<Fn>()(eastl::declval<variant_tagged_ref_t<Storage, 0>>()));

			template <size_t State>
			static EA_CONSTEXPR result_type call(Fn&& fn, Storage&& storage)
			{
				if constexpr (State < variant_valueless_states)
				{
					return static_cast<Fn&&>(fn)(variant_tagged<Storage&&, variant_npos>{static_cast<Storage&&>(storage)});
				}
				else
				{
					constexpr size_t I = State - variant_valueless_states;
					return static_cast<Fn&&>(fn)(
						variant_tagged_ref_t<Storage, I>{internal::variant_raw_get<I>(static_cast<Storage&&>(storage))});
				}
			}
		};

		template <class Storage, class Fn>
		EA_CONSTEXPR typename variant_raw_visit_dispatcher<Fn, Storage>::result_type variant_raw_visit(size_t index, Storage&& storage, Fn&& fn)
		{
			using dispatcher = variant_raw_visit_dispatcher<Fn, Storage>;
			return variant_dispatch<internal::variant_dispatch_strategy(dispatcher::states)>::template call<dispatcher>(
				index + variant_valueless_states, static_cast<Fn&&>(fn), static_cast<Storage&&>(storage));
		}

		// The data members, construction, destruction and assignment of a variant.
		template <class... Types>
		struct variant_base
		{
			using storage_t = variant_storage<Types...>;
			using index_t = variant_index_t<sizeof...(Types)>;

			storage_t mStorage;
			index_t mIndex;

			// The valueless state, only used by the copy and move constructors before they construct the alternative.
			EA_CPP20_CONSTEXPR variant_base() EA_NOEXCEPT : mStorage(), mIndex(index_t(-1)) {}

			template <size_t I, class... Args>
			EA_CONSTEXPR explicit variant_base(in_place_index_t<I>, Args&&... args)
				EA_NOEXCEPT(is_nothrow_constructible_v<meta::get_type_at_t<I, Types...>, Args...>)
				: mStorage(integral_constant<size_t, I>{}, eastl::forward<Args>(args)...), mIndex(static_cast<index_t>(I))
			{
			}

			EA_CONSTEXPR size_t index() const EA_NOEXCEPT
			{
				return static_cast<size_t>(mIndex);
			}

			EA_CONSTEXPR bool valueless_by_exception() const EA_NOEXCEPT
			{
			#if EASTL_EXCEPTIONS_ENABLED
				return mIndex < 0;
			#else
				return false;
			#endif
			}

			EA_CPP20_CONSTEXPR void set_valueless() EA_NOEXCEPT
			{
			#if EASTL_EXCEPTIONS_ENABLED
				mIndex = index_t(-1);
			#endif
			}

			// Destroys the active alternative, if any, and leaves the index unchanged.
			EA_CPP20_CONSTEXPR void destroy() EA_NOEXCEPT
			{
				if constexpr (!conjunction_v<is_trivially_destructible<Types>...>)
				{
					internal::variant_raw_visit(index(), mStorage, [](auto ref) EA_NOEXCEPT
					{
						if constexpr (decltype(ref)::index != variant_npos)
						{
							using alternative_t = remove_cvref_t<decltype(ref.value)>;
							ref.value.~alternative_t();
						}
					});
				}
			}

			EA_CPP20_CONSTEXPR void reset() EA_NOEXCEPT
			{
				destroy();
				set_valueless();
			}

			// pre: I == index()
			template <size_t I>
			EA_CPP20_CONSTEXPR void reset_alternative() EA_NOEXCEPT
			{
				if constexpr (I != variant_npos)
				{
					using alternative_t = remove_cv_t<meta::get_type_at_t<I, Types...>>;
					if constexpr (!is_trivially_destructible_v<alternative_t>)
						internal::variant_raw_get<I>(mStorage).~alternative_t();
					set_valueless();
				}
			}

			// pre: no active alternative (valueless, or destroyed)
			EA_DISABLE_VC_WARNING(4702) // unreachable code: the code after constructing an alternative whose constructor always throws
			template <size_t I, class... Args>
			EA_CPP20_CONSTEXPR void construct_valueless(Args&&... args)
			{
				internal::variant_construct_storage(mStorage, integral_constant<size_t, I>{}, eastl::forward<Args>(args)...);
				mIndex = static_cast<index_t>(I);
			}
			EA_RESTORE_VC_WARNING()

			// The special member functions used by EnableVariantOptionalSpecialMemberFunctions when they aren't trivial.
			EA_CPP20_CONSTEXPR void ConstructFrom(const variant_base& other)
				EA_NOEXCEPT(conjunction_v<is_nothrow_copy_constructible<Types>...>)
			{
				internal::variant_raw_visit(other.index(), other.mStorage, [this](auto ref)
				{
					if constexpr (decltype(ref)::index != variant_npos)
						this->template construct_valueless<decltype(ref)::index>(ref.value);
				});
			}

			EA_CPP20_CONSTEXPR void ConstructFrom(variant_base&& other)
				EA_NOEXCEPT(conjunction_v<is_nothrow_move_constructible<Types>...>)
			{
				internal::variant_raw_visit(other.index(), eastl::move(other.mStorage), [this](auto ref)
				{
					if constexpr (decltype(ref)::index != variant_npos)
						this->template construct_valueless<decltype(ref)::index>(static_cast<decltype(ref.value)&&>(ref.value));
				});
			}

			EA_CPP20_CONSTEXPR void AssignFrom(const variant_base& other)
				EA_NOEXCEPT(conjunction_v<is_nothrow_copy_constructible<Types>..., is_nothrow_copy_assignable<Types>...>)
			{
				internal::variant_raw_visit(other.index(), other.mStorage, [this](auto ref)
				{
					constexpr size_t I = decltype(ref)::index;
					if constexpr (I == variant_npos)
					{
						this->reset();
					}
					else if (this->index() == I)
					{
						internal::variant_raw_get<I>(this->mStorage) = ref.value;
					}
					else
					{
						using alternative_t = remove_cvref_t<decltype(ref.value)>;
						if constexpr (is_nothrow_copy_constructible_v<alternative_t> || !is_nothrow_move_constructible_v<alternative_t>)
						{
							this->reset();
							this->template construct_valueless<I>(ref.value);
						}
						else
						{
							// the copy can throw and the move can't: copy first, so a throwing copy leaves *this unchanged
							alternative_t temp(ref.value);
							this->reset();
							this->template construct_valueless<I>(eastl::move(temp));
						}
					}
				});
			}

			EA_CPP20_CONSTEXPR void AssignFrom(variant_base&& other)
				EA_NOEXCEPT(conjunction_v<is_nothrow_move_constructible<Types>..., is_nothrow_move_assignable<Types>...>)
			{
				internal::variant_raw_visit(other.index(), eastl::move(other.mStorage), [this](auto ref)
				{
					constexpr size_t I = decltype(ref)::index;
					if constexpr (I == variant_npos)
					{
						this->reset();
					}
					else if (this->index() == I)
					{
						internal::variant_raw_get<I>(this->mStorage) = static_cast<decltype(ref.value)&&>(ref.value);
					}
					else
					{
						this->reset();
						this->template construct_valueless<I>(static_cast<decltype(ref.value)&&>(ref.value));
					}
				});
			}
		};

		template <class... Types>
		struct variant_destroy_layer_impl : variant_base<Types...>
		{
			using variant_base<Types...>::variant_base;

			EA_CPP20_CONSTEXPR ~variant_destroy_layer_impl() { this->destroy(); }

			variant_destroy_layer_impl() = default;
			variant_destroy_layer_impl(const variant_destroy_layer_impl&) = default;
			variant_destroy_layer_impl(variant_destroy_layer_impl&&) = default;
			variant_destroy_layer_impl& operator=(const variant_destroy_layer_impl&) = default;
			variant_destroy_layer_impl& operator=(variant_destroy_layer_impl&&) = default;
		};

		// A variant of trivially destructible alternatives is trivially destructible.
		template <class... Types>
		using variant_destroy_layer = conditional_t<conjunction_v<is_trivially_destructible<Types>...>, variant_base<Types...>,
		                                            variant_destroy_layer_impl<Types...>>;

		// Finds swap through argument-dependent lookup only.
		namespace variant_adl_swap
		{
			void swap() = delete;

			template <class T, class = void>
			struct has_adl_swap : false_type {};

			template <class T>
			struct has_adl_swap<T, void_t<decltype(swap(eastl::declval<T&>(), eastl::declval<T&>()))>> : true_type {};
		}

		// Swapping the object representation of T is equivalent to swap(a, b) (MSVC STL's _Is_trivially_swappable).
		template <class T>
		struct variant_is_trivially_swappable
			: bool_constant<is_trivially_destructible_v<T> && is_trivially_move_constructible_v<T> &&
			                is_trivially_move_assignable_v<T> && !variant_adl_swap::has_adl_swap<T>::value>
		{
		};
	} // namespace internal


	///////////////////////////////////////////////////////////////////////////
	// 20.7.11, hash support
	//
	namespace internal {
	EA_DISABLE_VC_WARNING(4623) // default constructor was implicitly defined as deleted: that is how a disabled hash is disabled.
	template<class Variant, bool enabled>
	struct variant_hash : eastl::Internal::disabled_hash {};
	EA_RESTORE_VC_WARNING()

	template<class Variant>
	struct variant_hash<Variant, true>
	{
		size_t operator()(const Variant& obj) const
		{
			if (obj.valueless_by_exception())
				return 0;

			auto genericVisitor = [](const auto& thisAlternative)
			{
				using alternative_t = remove_const_t<remove_reference_t<decltype(thisAlternative)>>;
				return hash<alternative_t>{}(thisAlternative);
			};

			const size_t alternativeHash = visit(genericVisitor, obj);

			return (alternativeHash * 16777619) ^ obj.index();
		}
	};

	template<class, class, class = void>
	struct is_hasher_for : public false_type {};

	template<class Hash, class Key>
	struct is_hasher_for <Hash, Key, void_t<
		decltype(Hash()) // default constructible
		, decltype(Hash(declval<Hash>())) // copy constructible
		, decltype(declval<Hash>() = declval<const Hash&>()) // copy assignable
		, enable_if_t<is_swappable<Hash>::value> // swappable
		, decltype(size_t(declval<const Hash>()(declval<const Key&>()))) // function object
		>> : public true_type {};

	template <class Hash, class Key>
	constexpr bool is_hasher_for_v = is_hasher_for<Hash, Key>::value;
	} // namespace internal

	EA_DISABLE_VC_WARNING(4623) // default constructor was implicitly defined as deleted: that is how a disabled hash is disabled.
	template <class... Types>
	struct hash<variant<Types...>>
		: public internal::variant_hash<variant<Types...>, conjunction_v<internal::is_hasher_for<hash<remove_const_t<Types>>, Types>...>> {};
	EA_RESTORE_VC_WARNING()


	///////////////////////////////////////////////////////////////////////////
	// get_if
	//
	template <size_t I, class... Types, enable_if_t<I < sizeof...(Types), bool> = true>
	EA_CONSTEXPR add_pointer_t<variant_alternative_t<I, variant<Types...>>> get_if(variant<Types...>* pv) EA_NOEXCEPT
	{
		return (pv && pv->index() == I) ? eastl::addressof(internal::variant_raw_get<I>(internal::variant_access::storage(*pv))) : nullptr;
	}

	template <size_t I, class... Types, enable_if_t<I < sizeof...(Types), bool> = true>
	EA_CONSTEXPR add_pointer_t<const variant_alternative_t<I, variant<Types...>>> get_if(const variant<Types...>* pv) EA_NOEXCEPT
	{
		return (pv && pv->index() == I) ? eastl::addressof(internal::variant_raw_get<I>(internal::variant_access::storage(*pv))) : nullptr;
	}

	template <class T, class... Types, size_t I = meta::get_type_index_v<T, Types...>, enable_if_t<I < sizeof...(Types) && meta::duplicate_type_check_v<T, Types...>, bool> = true>
	EA_CONSTEXPR add_pointer_t<T> get_if(variant<Types...>* pv) EA_NOEXCEPT
	{
		return get_if<I>(pv);
	}

	template <class T, class... Types, size_t I = meta::get_type_index_v<T, Types...>, enable_if_t<I < sizeof...(Types) && meta::duplicate_type_check_v<T, Types...>, bool> = true>
	EA_CONSTEXPR add_pointer_t<const T> get_if(const variant<Types...>* pv) EA_NOEXCEPT
	{
		return get_if<I>(pv);
	}


	///////////////////////////////////////////////////////////////////////////
	// get
	//
	template <size_t I, class... Types, enable_if_t<I < sizeof...(Types), bool> = true>
	EA_CONSTEXPR variant_alternative_t<I, variant<Types...>>& get(variant<Types...>& v)
	{
		if (v.index() != I)
			EASTL_THROW_OR_ASSERT(bad_variant_access, "get -- bad_variant_access");

		return internal::variant_raw_get<I>(internal::variant_access::storage(v));
	}

	template <size_t I, class... Types, enable_if_t<I < sizeof...(Types), bool> = true>
	EA_CONSTEXPR variant_alternative_t<I, variant<Types...>>&& get(variant<Types...>&& v)
	{
		if (v.index() != I)
			EASTL_THROW_OR_ASSERT(bad_variant_access, "get -- bad_variant_access");

		return internal::variant_raw_get<I>(internal::variant_access::storage(eastl::move(v)));
	}

	template <size_t I, class... Types, enable_if_t<I < sizeof...(Types), bool> = true>
	EA_CONSTEXPR const variant_alternative_t<I, variant<Types...>>& get(const variant<Types...>& v)
	{
		if (v.index() != I)
			EASTL_THROW_OR_ASSERT(bad_variant_access, "get -- bad_variant_access");

		return internal::variant_raw_get<I>(internal::variant_access::storage(v));
	}

	template <size_t I, class... Types, enable_if_t<I < sizeof...(Types), bool> = true>
	EA_CONSTEXPR const variant_alternative_t<I, variant<Types...>>&& get(const variant<Types...>&& v)
	{
		if (v.index() != I)
			EASTL_THROW_OR_ASSERT(bad_variant_access, "get -- bad_variant_access");

		return internal::variant_raw_get<I>(internal::variant_access::storage(eastl::move(v)));
	}

	template <class T, class... Types, size_t I = meta::get_type_index_v<T, Types...>, enable_if_t<I < sizeof...(Types) && meta::duplicate_type_check_v<T, Types...>, bool> = true>
	EA_CONSTEXPR T& get(variant<Types...>& v)
	{
		return get<I>(v);
	}

	template <class T, class... Types, size_t I = meta::get_type_index_v<T, Types...>, enable_if_t<I < sizeof...(Types) && meta::duplicate_type_check_v<T, Types...>, bool> = true>
	EA_CONSTEXPR T&& get(variant<Types...>&& v)
	{
		return get<I>(eastl::move(v));
	}

	template <class T, class... Types, size_t I = meta::get_type_index_v<T, Types...>, enable_if_t<I < sizeof...(Types) && meta::duplicate_type_check_v<T, Types...>, bool> = true>
	EA_CONSTEXPR const T& get(const variant<Types...>& v)
	{
		return get<I>(v);
	}

	template <class T, class... Types, size_t I = meta::get_type_index_v<T, Types...>, enable_if_t<I < sizeof...(Types) && meta::duplicate_type_check_v<T, Types...>, bool> = true>
	EA_CONSTEXPR const T&& get(const variant<Types...>&& v)
	{
		return get<I>(eastl::move(v));
	}


	///////////////////////////////////////////////////////////////////////////
	// 20.7.4, value access
	//
	template <class T, class... Types, size_t I = meta::get_type_index_v<T, Types...>, enable_if_t<I < sizeof...(Types) && meta::duplicate_type_check_v<T, Types...>, bool> = true>
	EA_CONSTEXPR bool holds_alternative(const variant<Types...>& v) EA_NOEXCEPT
	{
		return I == variant_npos ? false : (v.index() == I);
	}


	namespace internal {
	template<typename>
	constexpr bool is_in_place_type_specialization_v = false;

	template<typename T>
	constexpr bool is_in_place_type_specialization_v<in_place_type_t<T>> = true;

	template<typename>
	constexpr bool is_in_place_index_specialization_v = false;

	template<size_t Idx>
	constexpr bool is_in_place_index_specialization_v<in_place_index_t<Idx>> = true;

	// Model the imaginary overload set with array list-initialization so that narrowing
	// conversions are excluded. Keep one overload per alternative: duplicate types must
	// remain ambiguous rather than being collapsed into a single candidate.
	template <typename T>
	void variant_conversion_array(T (&&)[1]);

	template <typename T, typename Source, size_t I, typename = void>
	struct variant_conversion_overload
	{
		static void select();
	};

	template <typename T, typename Source, size_t I>
	struct variant_conversion_overload<T, Source, I,
		void_t<decltype(variant_conversion_array<T>({eastl::declval<Source>()}))>>
	{
		static integral_constant<size_t, I> select(remove_cv_t<T>);
	};

	template <typename Source, typename Indices, typename... Types>
	struct variant_conversion_overloads;

	template <typename Source, size_t... Indices, typename... Types>
	struct variant_conversion_overloads<Source, index_sequence<Indices...>, Types...>
		: variant_conversion_overload<Types, Source, Indices>...
	{
		using variant_conversion_overload<Types, Source, Indices>::select...;
	};

	template <typename Source, typename... Types>
	using variant_conversion_index_t = decltype(
		variant_conversion_overloads<Source, index_sequence_for<Types...>, Types...>::select(
			eastl::declval<Source>()));
	} // namespace internal


	///////////////////////////////////////////////////////////////////////////
	// 20.7.2, variant
	//
	template <class... Types>
	class variant
		: private internal::EnableVariantOptionalSpecialMemberFunctions<internal::variant_destroy_layer<Types...>, Types...>
	{
		using base_type = internal::EnableVariantOptionalSpecialMemberFunctions<internal::variant_destroy_layer<Types...>, Types...>;

		using T_0 = variant_alternative_t<0, variant>;  // alias for the 1st type in the variadic pack

		static_assert(sizeof...(Types) > 0,                           "variant must have at least 1 type (empty variants are ill-formed)");
		static_assert(disjunction_v<is_reference<Types>...> == false, "variant alternative types must be destructible (non-reference)");
		static_assert(disjunction_v<is_array<Types>...> == false,     "variant alternative types must be destructible (non-array)");
		static_assert(conjunction_v<is_destructible<Types>...>,		  "variant alternative types must be destructible");

		friend struct internal::variant_access;

	public:

		///////////////////////////////////////////////////////////////////////////
		// 20.7.2.1, constructors
		//

		// Only participates in overload resolution when the first alternative is default constructible
		template <typename TT0 = T_0, enable_if_t<is_default_constructible_v<TT0>, bool> = true>
		EA_CONSTEXPR variant() EA_NOEXCEPT(eastl::is_nothrow_default_constructible_v<TT0>)
			: base_type(in_place_index<0>)
		{
		}

		// implemented by EnableVariantOptionalSpecialMemberFunctions<...>
		variant(const variant&) = default;
		variant(variant&&) = default;

		// Conversion constructor
		template <typename T,
			typename Index = internal::variant_conversion_index_t<T&&, Types...>,
			size_t I = Index::value,
			typename T_j = variant_alternative_t<I, variant>,
			enable_if_t<(sizeof...(Types) > 0) &&
				I < sizeof...(Types) &&
				!is_same_v<remove_cvref_t<T>, variant> &&
				!internal::is_in_place_type_specialization_v<remove_cvref_t<T>> &&
				!internal::is_in_place_index_specialization_v<remove_cvref_t<T>> &&
				eastl::is_constructible_v<T_j, T>, bool> = true>
			EA_CONSTEXPR variant(T&& t) EA_NOEXCEPT(is_nothrow_constructible_v<T_j, T>)
			: base_type(in_place_index<I>, eastl::forward<T>(t))
		{
		}

		///////////////////////////////////////////////////////////////////////////
		// 20.7.2.1, in_place_t constructors
		//
		template <
			class T,
			class... Args,
			enable_if_t<conjunction_v<meta::duplicate_type_check<T, Types...>, is_constructible<T, Args...>>, bool> = true>
			EA_CONSTEXPR explicit variant(in_place_type_t<T>, Args&&... args) EA_NOEXCEPT(is_nothrow_constructible_v<T, Args...>)
			: base_type(in_place_index<meta::get_type_index_v<T, Types...>>, eastl::forward<Args>(args)...)
		{
		}

		template <
			class T,
			class U,
			class... Args,
			enable_if_t<conjunction_v<meta::duplicate_type_check<T, Types...>,
				is_constructible<T, std::initializer_list<U>&, Args...>>, bool> = true>
			EA_CONSTEXPR explicit variant(in_place_type_t<T>, std::initializer_list<U> il, Args&&... args)
				EA_NOEXCEPT(is_nothrow_constructible_v<T, std::initializer_list<U>&, Args...>)
			: base_type(in_place_index<meta::get_type_index_v<T, Types...>>, il, eastl::forward<Args>(args)...)
		{
		}

		template <size_t I,
			class... Args,
			enable_if_t<(I < sizeof...(Types)), bool> = true,
			enable_if_t<is_constructible_v<meta::get_type_at_t<I, Types...>, Args...>, bool> = true>
			EA_CONSTEXPR explicit variant(in_place_index_t<I>, Args&&... args)
				EA_NOEXCEPT(is_nothrow_constructible_v<meta::get_type_at_t<I, Types...>, Args...>)
			: base_type(in_place_index<I>, eastl::forward<Args>(args)...)
		{
		}

		template <size_t I,
			class U,
			class... Args,
			enable_if_t<(I < sizeof...(Types)), bool> = true,
			enable_if_t<is_constructible_v<meta::get_type_at_t<I, Types...>, std::initializer_list<U>&, Args...>, bool> = true>
			EA_CONSTEXPR explicit variant(in_place_index_t<I>, std::initializer_list<U> il, Args&&... args)
				EA_NOEXCEPT(is_nothrow_constructible_v<meta::get_type_at_t<I, Types...>, std::initializer_list<U>&, Args...>)
			: base_type(in_place_index<I>, il, eastl::forward<Args>(args)...)
		{
		}

		///////////////////////////////////////////////////////////////////////////
		// 20.7.2.2, destructor
		//
		// implemented by EnableVariantOptionalSpecialMemberFunctions<...>
		~variant() = default;


		///////////////////////////////////////////////////////////////////////////
		// 20.7.2.4, modifiers
		//

		// Equivalent to emplace<I>(std::forward<Args>(args)...), where I is the zero-based index of T in Types....
		// This overload only participates in overload resolution if std::is_constructible_v<T, Args...> is true, and T
		// occurs exactly once in Types...
		template <
		    class T,
		    class... Args,
		    size_t I = meta::get_type_index_v<T, Types...>,
		    enable_if_t<conjunction_v<is_constructible<T, Args...>, meta::duplicate_type_check<T, Types...>>, bool> = true>
		EA_CPP20_CONSTEXPR T& emplace(Args&&... args) EA_NOEXCEPT(is_nothrow_constructible_v<T, Args...>)
		{
			return emplace<I>(eastl::forward<Args>(args)...);
		}

		// Equivalent to emplace<I>(il, std::forward<Args>(args)...), where I is the zero-based index of T in Types....
		// This overload only participates in overload resolution if std::is_constructible_v<T,
		// std::initializer_list<U>&, Args...> is true, and T occurs exactly once in Types...
		template <class T,
		          class U,
		          class... Args,
		          size_t I = meta::get_type_index_v<T, Types...>,
		          enable_if_t<conjunction_v<is_constructible<T, std::initializer_list<U>&, Args...>,
		                                               meta::duplicate_type_check<T, Types...>>, bool> = true>
		EA_CPP20_CONSTEXPR T& emplace(std::initializer_list<U> il, Args&&... args)
			EA_NOEXCEPT(is_nothrow_constructible_v<T, std::initializer_list<U>&, Args...>)
		{
			return emplace<I>(il, eastl::forward<Args>(args)...);
		}

		// First, destroys the currently contained value (if any). Then direct-initializes the contained value as if
		// constructing a value of type T_I with the arguments std::forward<Args>(args).... If an exception is thrown,
		// *this may become valueless_by_exception. This overload only participates in overload resolution if
		// std::is_constructible_v<T_I, Args...> is true. The behavior is undefined if I is not less than
		// sizeof...(Types).
		//
		EA_DISABLE_VC_WARNING(4702) // unreachable code: the code after constructing an alternative whose constructor always throws
		template <size_t I,
		          class... Args,
		          typename T = meta::get_type_at_t<I, Types...>,
		          enable_if_t<is_constructible_v<T, Args...>, bool> = true>
		EA_CPP20_CONSTEXPR variant_alternative_t<I, variant>& emplace(Args&&... args) EA_NOEXCEPT(is_nothrow_constructible_v<T, Args...>)
		{
			this->reset();
			this->template construct_valueless<I>(eastl::forward<Args>(args)...);
			return internal::variant_raw_get<I>(this->mStorage);
		}

		// First, destroys the currently contained value (if any). Then direct-initializes the contained value as if
		// constructing a value of type T_I with the arguments il, std::forward<Args>(args).... If an exception is
		// thrown, *this may become valueless_by_exception. This overload only participates in overload resolution if
		// std::is_constructible_v<T_I, initializer_list<U>&, Args...> is true. The behavior is undefined if I is not
		// less than sizeof...(Types).
		//
		template <size_t I,
		          class U,
		          class... Args,
		          typename T = meta::get_type_at_t<I, Types...>,
		          enable_if_t<is_constructible_v<T, std::initializer_list<U>&, Args...>, bool> = true>
		EA_CPP20_CONSTEXPR variant_alternative_t<I, variant>& emplace(std::initializer_list<U> il, Args&&... args)
			EA_NOEXCEPT(is_nothrow_constructible_v<T, std::initializer_list<U>&, Args...>)
		{
			this->reset();
			this->template construct_valueless<I>(il, eastl::forward<Args>(args)...);
			return internal::variant_raw_get<I>(this->mStorage);
		}
		EA_RESTORE_VC_WARNING() // 4702


		///////////////////////////////////////////////////////////////////////////
		// 20.7.2.3, assignment
		//

		// implemented by EnableVariantOptionalSpecialMemberFunctions<...>
		variant& operator=(const variant&) = default;
		variant& operator=(variant&&) = default;

		template <class T,
		          typename Index = internal::variant_conversion_index_t<T&&, Types...>,
		          size_t I = Index::value,
		          typename T_j = variant_alternative_t<I, variant>,
		          enable_if_t<!eastl::is_same_v<eastl::remove_cvref_t<T>, variant> && eastl::is_assignable_v<T_j&, T> &&
		              eastl::is_constructible_v<T_j, T>, bool> = true>
		EA_CPP20_CONSTEXPR variant& operator=(T&& t)
		    EA_NOEXCEPT(conjunction_v<is_nothrow_assignable<T_j&, T>, is_nothrow_constructible<T_j, T>>)
		{
			if (index() == I) // already holds a T_j
			{
				internal::variant_raw_get<I>(this->mStorage) = eastl::forward<T>(t);
			}
			else if constexpr (is_nothrow_constructible_v<T_j, T> || !is_nothrow_move_constructible_v<T_j>)
			{
				this->reset();
				this->template construct_valueless<I>(eastl::forward<T>(t));
			}
			else
			{
				// the conversion can throw and the move can't: convert first, so a throwing conversion leaves *this unchanged
				T_j temp(eastl::forward<T>(t));
				this->reset();
				this->template construct_valueless<I>(eastl::move(temp));
			}

			return *this;
		}


		///////////////////////////////////////////////////////////////////////////
		// 20.7.2.5, value status
		//
		using base_type::index;
		using base_type::valueless_by_exception;


		///////////////////////////////////////////////////////////////////////////
		// 20.7.2.6, swap
		//
		EA_CPP20_CONSTEXPR void swap(variant& other)
			EA_NOEXCEPT(conjunction_v<is_nothrow_move_constructible<Types>..., is_nothrow_swappable<Types>...>)
		{
			if constexpr (conjunction_v<internal::variant_is_trivially_swappable<Types>...>)
			{
				// swap the object representations (the index included)
				using swap_base = internal::variant_base<Types...>;
				eastl::swap(static_cast<swap_base&>(*this), static_cast<swap_base&>(other));
			}
			else if constexpr (sizeof...(Types) < 32)
			{
				// visit both variants (quadratic code size, so only for smaller variants)
				internal::variant_raw_visit(index(), this->mStorage, [this, &other](auto thisRef)
				{
					internal::variant_raw_visit(other.index(), other.mStorage, [this, &other, thisRef](auto otherRef)
					{
						(void)thisRef; // unused when both variants are valueless
						constexpr size_t ThisIndex = decltype(thisRef)::index;
						constexpr size_t OtherIndex = decltype(otherRef)::index;

						if constexpr (ThisIndex == OtherIndex)
						{
							if constexpr (ThisIndex != variant_npos)
							{
								using eastl::swap;
								swap(thisRef.value, otherRef.value);
							}
						}
						else if constexpr (ThisIndex == variant_npos)
						{
							this->template construct_valueless<OtherIndex>(eastl::move(otherRef.value));
							other.template reset_alternative<OtherIndex>();
						}
						else if constexpr (OtherIndex == variant_npos)
						{
							other.template construct_valueless<ThisIndex>(eastl::move(thisRef.value));
							this->template reset_alternative<ThisIndex>();
						}
						else
						{
							auto temp = eastl::move(thisRef.value);
							this->template reset_alternative<ThisIndex>();
							this->template construct_valueless<OtherIndex>(eastl::move(otherRef.value));
							other.template reset_alternative<OtherIndex>();
							other.template construct_valueless<ThisIndex>(eastl::move(temp));
						}
					});
				});
			}
			else
			{
				if (index() == other.index())
				{
					internal::variant_raw_visit(index(), other.mStorage, [this](auto otherRef)
					{
						constexpr size_t I = decltype(otherRef)::index;
						if constexpr (I != variant_npos)
						{
							using eastl::swap;
							swap(internal::variant_raw_get<I>(this->mStorage), otherRef.value);
						}
					});
				}
				else
				{
					variant temp = eastl::move(*this);
					emplace_from(eastl::move(other));
					other.emplace_from(eastl::move(temp));
				}
			}
		}

	private:
		// Moves other's alternative into *this, whatever *this holds.
		EA_CPP20_CONSTEXPR void emplace_from(variant&& other) EA_NOEXCEPT(conjunction_v<is_nothrow_move_constructible<Types>...>)
		{
			this->reset();
			internal::variant_raw_visit(other.index(), other.mStorage, [this](auto ref)
			{
				constexpr size_t I = decltype(ref)::index;
				if constexpr (I != variant_npos)
					this->template construct_valueless<I>(eastl::move(ref.value));
			});
		}
	};

	///////////////////////////////////////////////////////////////////////////
	// 20.7.9, swap
	//
	// Participates in overload resolution only if every alternative is move constructible and swappable.
	//
	template <class... Types, typename enable_if<conjunction_v<is_move_constructible<Types>..., is_swappable<Types>...>, int>::type = 0>
	EA_CPP20_CONSTEXPR void swap(variant<Types...>& lhs, variant<Types...>& rhs)
		EA_NOEXCEPT(EA_NOEXCEPT(lhs.swap(rhs)))
	{
		lhs.swap(rhs);
	}


	///////////////////////////////////////////////////////////////////////////
	// 20.7.6, visitation
	//
	// Example:
	//     struct MyVisitor
	//     {
	//         auto operator()(int) {};
	//         auto operator()(long) {};
	//         auto operator()(string) {};
	//     };
	//
	//     variant<int, long, string> v = "Hello, Variant";
	//     visit(MyVisitor{}, v);  // calls MyVisitor::operator()(string) {}
	//
	// Visiting several variants calls the visitor with one alternative of each:
	//
	//     variant<int, string> a = 42;
	//     variant<int, string> b = "hello";
	//     visit(MyVisitor{}, a, b); // calls MyVisitor::operator()(int, string)
	//
	namespace internal
	{
		// visit() overloads participate in overload resolution only if every argument is a variant or derives from one.
		template<typename ... Ts> variant<Ts...>& as_variant_impl(variant<Ts...>&);
		template<typename ... Ts> const variant<Ts...>& as_variant_impl(const variant<Ts...>&);
		template<typename ... Ts> variant<Ts...>&& as_variant_impl(variant<Ts...>&&);
		template<typename ... Ts> const variant<Ts...>&& as_variant_impl(const variant<Ts...>&&);
		template<typename T> using as_variant = decltype(as_variant_impl(declval<T>()));

		template <class Visitor, class... Variants>
		using variant_visit_result_t = decltype(eastl::invoke(eastl::declval<Visitor>(),
			internal::variant_raw_get<0>(internal::variant_access::storage(eastl::declval<Variants>()))...));

		// The combined state of the variants: a mixed-radix number with one digit per variant.
		template <class... Variants>
		EA_CONSTEXPR size_t variant_visit_state(const Variants&... variants) EA_NOEXCEPT
		{
			size_t state = 0;
			((state = state * (variant_size_v<Variants> + variant_valueless_states) + (variants.index() + variant_valueless_states)), ...);
			return state;
		}

		// Calls the visitor with the alternatives of combined state State. SameResult checks that every combination of
		// alternatives gives the result type (visit), or the result is converted to R (visit<R>).
		template <class R, bool SameResult, class Visitor, class... Variants>
		struct variant_visit_dispatcher
		{
			static EA_CONSTEXPR size_t states = (size_t(1) * ... * (variant_size_v<remove_cvref_t<Variants>> + variant_valueless_states));
			static EA_CONSTEXPR size_t counts[] = {(variant_size_v<remove_cvref_t<Variants>> + variant_valueless_states)..., 1};
			using result_type = R;

			// The state of variant J in combined state K (the inverse of variant_visit_state).
			static EA_CONSTEXPR size_t state_of(size_t k, size_t j)
			{
				size_t stride = 1;
				for (size_t l = j + 1; l < sizeof...(Variants); ++l)
					stride *= counts[l];
				return (k / stride) % counts[j];
			}

			template <size_t State, size_t... Js>
			static EA_CONSTEXPR R call_state(index_sequence<Js...>, Visitor&& visitor, Variants&&... variants)
			{
				if constexpr (((state_of(State, Js) < variant_valueless_states) || ...))
				{
					EA_UNUSED(visitor);
					((void)variants, ...);
				#if EASTL_EXCEPTIONS_ENABLED
					throw bad_variant_access();
				#endif
				}
				else
				{
					static_assert(!SameResult ||
						is_same_v<R, decltype(eastl::invoke(static_cast<Visitor&&>(visitor),
							internal::variant_raw_get<state_of(State, Js) - variant_valueless_states>(
								internal::variant_access::storage(static_cast<Variants&&>(variants)))...))>,
						"`visit` requires the visitor to have the same return type for all alternatives of the variants");

					if constexpr (is_void_v<R>)
						return static_cast<void>(eastl::invoke(static_cast<Visitor&&>(visitor),
							internal::variant_raw_get<state_of(State, Js) - variant_valueless_states>(
								internal::variant_access::storage(static_cast<Variants&&>(variants)))...));
					else
						return eastl::invoke(static_cast<Visitor&&>(visitor),
							internal::variant_raw_get<state_of(State, Js) - variant_valueless_states>(
								internal::variant_access::storage(static_cast<Variants&&>(variants)))...);
				}
			}

			template <size_t State>
			static EA_CONSTEXPR R call(Visitor&& visitor, Variants&&... variants)
			{
				return call_state<State>(make_index_sequence<sizeof...(Variants)>{}, static_cast<Visitor&&>(visitor),
				                         static_cast<Variants&&>(variants)...);
			}
		};

		template <class R, bool SameResult, class Visitor, class... Variants>
		EA_CONSTEXPR R variant_visit(Visitor&& visitor, Variants&&... variants)
		{
			using dispatcher = variant_visit_dispatcher<R, SameResult, Visitor, Variants...>;
			return variant_dispatch<internal::variant_dispatch_strategy(dispatcher::states)>::template call<dispatcher>(
				internal::variant_visit_state(variants...), static_cast<Visitor&&>(visitor), static_cast<Variants&&>(variants)...);
		}
	} // namespace internal

	template <typename Visitor, typename... Variants, typename = eastl::void_t<internal::as_variant<Variants>...>>
	EA_CONSTEXPR internal::variant_visit_result_t<Visitor, internal::as_variant<Variants>...> visit(Visitor&& visitor, Variants&&... variants)
	{
		using result_type = internal::variant_visit_result_t<Visitor, internal::as_variant<Variants>...>;
		return internal::variant_visit<result_type, true>(eastl::forward<Visitor>(visitor),
		                                                  static_cast<internal::as_variant<Variants>&&>(variants)...);
	}

	// C++20: overload with a user specifiable return type
	template <typename R, typename Visitor, typename... Variants, typename = eastl::void_t<internal::as_variant<Variants>...>>
	EA_CONSTEXPR R visit(Visitor&& visitor, Variants&&... variants)
	{
		return internal::variant_visit<R, false>(eastl::forward<Visitor>(visitor),
		                                         static_cast<internal::as_variant<Variants>&&>(variants)...);
	}


	///////////////////////////////////////////////////////////////////////////
	// 20.7.5, relational operators
	//
	namespace internal
	{
		// Compares the alternative of rhs with the same alternative of lhs (the indices are known to be equal).
		EA_DISABLE_VC_WARNING(4623) // default constructor was implicitly defined as deleted: the reference member has no default.
		template <class Compare, class R, class... Types>
		struct variant_relational_visitor
		{
			const variant_storage<Types...>& mLhs;

			template <class T, size_t I>
			EA_CONSTEXPR R operator()(variant_tagged<const T&, I> rhs) const
			{
				if constexpr (I == variant_npos)
					return Compare{}(0, 0); // both valueless: as if equal
				else
					return Compare{}(internal::variant_raw_get<I>(mLhs), rhs.value);
			}
		};
		EA_RESTORE_VC_WARNING()
	} // namespace internal

	template <class... Types>
	EA_CONSTEXPR bool operator==(const variant<Types...>& lhs, const variant<Types...>& rhs)
		EA_NOEXCEPT(conjunction_v<is_nothrow_invocable_r<bool, equal_to<>, const Types&, const Types&>...>)
	{
		const size_t rhsIndex = rhs.index();
		return lhs.index() == rhsIndex &&
		       internal::variant_raw_visit(rhsIndex, internal::variant_access::storage(rhs),
		           internal::variant_relational_visitor<equal_to<>, bool, Types...>{internal::variant_access::storage(lhs)});
	}

	template <class... Types>
	EA_CONSTEXPR bool operator!=(const variant<Types...>& lhs, const variant<Types...>& rhs)
		EA_NOEXCEPT(conjunction_v<is_nothrow_invocable_r<bool, not_equal_to<>, const Types&, const Types&>...>)
	{
		const size_t rhsIndex = rhs.index();
		return lhs.index() != rhsIndex ||
		       internal::variant_raw_visit(rhsIndex, internal::variant_access::storage(rhs),
		           internal::variant_relational_visitor<not_equal_to<>, bool, Types...>{internal::variant_access::storage(lhs)});
	}

	// The ordering operators order a valueless variant first: its index plus the valueless state count is 0.
	template <class... Types>
	EA_CONSTEXPR bool operator<(const variant<Types...>& lhs, const variant<Types...>& rhs)
		EA_NOEXCEPT(conjunction_v<is_nothrow_invocable_r<bool, less<>, const Types&, const Types&>...>)
	{
		const size_t lhsOffset = lhs.index() + internal::variant_valueless_states;
		const size_t rhsOffset = rhs.index() + internal::variant_valueless_states;
		return lhsOffset < rhsOffset ||
		       (lhsOffset == rhsOffset &&
		        internal::variant_raw_visit(rhs.index(), internal::variant_access::storage(rhs),
		            internal::variant_relational_visitor<less<>, bool, Types...>{internal::variant_access::storage(lhs)}));
	}

	template <class... Types>
	EA_CONSTEXPR bool operator>(const variant<Types...>& lhs, const variant<Types...>& rhs)
		EA_NOEXCEPT(conjunction_v<is_nothrow_invocable_r<bool, greater<>, const Types&, const Types&>...>)
	{
		const size_t lhsOffset = lhs.index() + internal::variant_valueless_states;
		const size_t rhsOffset = rhs.index() + internal::variant_valueless_states;
		return lhsOffset > rhsOffset ||
		       (lhsOffset == rhsOffset &&
		        internal::variant_raw_visit(rhs.index(), internal::variant_access::storage(rhs),
		            internal::variant_relational_visitor<greater<>, bool, Types...>{internal::variant_access::storage(lhs)}));
	}

	template <class... Types>
	EA_CONSTEXPR bool operator<=(const variant<Types...>& lhs, const variant<Types...>& rhs)
		EA_NOEXCEPT(conjunction_v<is_nothrow_invocable_r<bool, less_equal<>, const Types&, const Types&>...>)
	{
		const size_t lhsOffset = lhs.index() + internal::variant_valueless_states;
		const size_t rhsOffset = rhs.index() + internal::variant_valueless_states;
		return lhsOffset < rhsOffset ||
		       (lhsOffset == rhsOffset &&
		        internal::variant_raw_visit(rhs.index(), internal::variant_access::storage(rhs),
		            internal::variant_relational_visitor<less_equal<>, bool, Types...>{internal::variant_access::storage(lhs)}));
	}

	template <class... Types>
	EA_CONSTEXPR bool operator>=(const variant<Types...>& lhs, const variant<Types...>& rhs)
		EA_NOEXCEPT(conjunction_v<is_nothrow_invocable_r<bool, greater_equal<>, const Types&, const Types&>...>)
	{
		const size_t lhsOffset = lhs.index() + internal::variant_valueless_states;
		const size_t rhsOffset = rhs.index() + internal::variant_valueless_states;
		return lhsOffset > rhsOffset ||
		       (lhsOffset == rhsOffset &&
		        internal::variant_raw_visit(rhs.index(), internal::variant_access::storage(rhs),
		            internal::variant_relational_visitor<greater_equal<>, bool, Types...>{internal::variant_access::storage(lhs)}));
	}

#if defined(EA_COMPILER_HAS_THREE_WAY_COMPARISON)
	template <class... Types> requires (std::three_way_comparable<Types> && ...)
	EA_CONSTEXPR std::common_comparison_category_t<std::compare_three_way_result_t<Types>...> operator<=>(const variant<Types...>& lhs, const variant<Types...>& rhs)
		EA_NOEXCEPT(conjunction_v<is_nothrow_invocable_r<std::common_comparison_category_t<std::compare_three_way_result_t<Types>...>,
			std::compare_three_way, const Types&, const Types&>...>)
	{
		using result_type = std::common_comparison_category_t<std::compare_three_way_result_t<Types>...>;
		const size_t lhsOffset = lhs.index() + internal::variant_valueless_states;
		const size_t rhsOffset = rhs.index() + internal::variant_valueless_states;
		const auto offsetOrder = lhsOffset <=> rhsOffset;
		return offsetOrder != 0
			? offsetOrder
			: internal::variant_raw_visit(rhs.index(), internal::variant_access::storage(rhs),
				internal::variant_relational_visitor<std::compare_three_way, result_type, Types...>{internal::variant_access::storage(lhs)});
	}
#endif

} // namespace eastl

EA_RESTORE_VC_WARNING()

#endif // EASTL_VARIANT_H
