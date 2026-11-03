///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// This file implements sorting algorithms. Some of these are equivalent to 
// std C++ sorting algorithms, while others don't have equivalents in the 
// C++ standard. We implement the following sorting algorithms:
//    is_sorted             -- 
//    sort                  -- Unstable.    The implementation of this is mapped to quick_sort by default.
//    quick_sort            -- Unstable.    This is actually an intro-sort (quick sort with switch to insertion sort).
//    tim_sort              -- Stable.
//    tim_sort_buffer       -- Stable.
//    partial_sort          -- Unstable.
//    insertion_sort        -- Stable. 
//    shell_sort            -- Unstable.
//    heap_sort             -- Unstable. 
//    stable_sort           -- Stable.      The implementation of this is simply mapped to merge_sort.
//    merge                 -- 
//    merge_sort            -- Stable. 
//    merge_sort_buffer     -- Stable. 
//    nth_element           -- Unstable.
//    radix_sort            -- Stable.      Important and useful sort for integral data, and faster than all others for this.
//    comb_sort             -- Unstable.    Possibly the best combination of small code size but fast sort.
//    bubble_sort           -- Stable.      Useful in practice for sorting tiny sets of data (<= 10 elements).
//    selection_sort*       -- Unstable.
//    shaker_sort*          -- Stable.
//    bucket_sort*          -- Stable. 
//
// * Found in sort_extra.h.
//
// Additional sorting and related algorithms we may want to implement:
//    partial_sort_copy     This would be like the std STL version.
//    paritition            This would be like the std STL version. This is not categorized as a sort routine by the language standard.
//    stable_partition      This would be like the std STL version.
//    counting_sort         Maybe we don't want to implement this.
//
//////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_SORT_H
#define EASTL_SORT_H


#include <EASTL/internal/config.h>
#include <EASTL/internal/move_help.h>
#include <EASTL/bit.h>
#include <EASTL/iterator.h>
#include <EASTL/memory.h>
#include <EASTL/algorithm.h>
#include <EASTL/functional.h>
#include <EASTL/heap.h>
#include <EASTL/allocator.h>
#include <EASTL/memory.h>


#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif


// EASTL_PLATFORM_PREFERRED_ALIGNMENT
//
// Allows for slightly faster buffers in some cases.
//
#if !defined(EASTL_PLATFORM_PREFERRED_ALIGNMENT)
	#if defined(EA_PROCESSOR_ARM)
		#define EASTL_PLATFORM_PREFERRED_ALIGNMENT 8
	#else
		#define EASTL_PLATFORM_PREFERRED_ALIGNMENT 16
	#endif
#endif


namespace eastl
{

	/// is_sorted
	///
	/// Returns true if the range [first, last) is sorted.
	/// An empty range is considered to be sorted.
	/// To test if a range is reverse-sorted, use 'greater' as the comparison 
	/// instead of 'less'.
	///
	/// Example usage:
	///    vector<int> intArray;
	///    bool bIsSorted        = is_sorted(intArray.begin(), intArray.end());
	///    bool bIsReverseSorted = is_sorted(intArray.begin(), intArray.end(), greater<int>());
	///
	template <typename ForwardIterator, typename StrictWeakOrdering>
	bool is_sorted(ForwardIterator first, ForwardIterator last, StrictWeakOrdering compare)
	{
		if(first != last)
		{
			ForwardIterator current = first;

			for(++current; current != last; first = current, (void)++current)
			{
				if(compare(*current, *first))
				{
					EASTL_VALIDATE_COMPARE(!compare(*first, *current)); // Validate that the compare function is sane.
					return false;
				}
			}
		}
		return true;
	}

	template <typename ForwardIterator>
	inline bool is_sorted(ForwardIterator first, ForwardIterator last)
	{
		typedef eastl::less<typename eastl::iterator_traits<ForwardIterator>::value_type> Less;

		return eastl::is_sorted<ForwardIterator, Less>(first, last, Less());
	}



	/// is_sorted_until
	///
	/// Returns an iterator to the first element in the range [first,last) which does not follow an ascending order.
	/// The range between first and the iterator returned is sorted.
	/// If the entire range is sorted, the function returns last.
	/// The elements are compared using operator< for the first version, and comp for the second.
	///
	/// Example usage:
	///     vector<int> intArray;
	///     vector<int>::iterator unsorted_element = is_sorted_until(eastl::end(intArray), eastl::end(intArray));
	///     vector<int>::iterator unsorted_element_with_user_compare = is_sorted_until(eastl::end(intArray), eastl::end(intArray), eastl::less<int>());
	///
	template<typename ForwardIterator>
	ForwardIterator is_sorted_until(ForwardIterator first, ForwardIterator last)
	{
		if(first != last)
		{
			ForwardIterator next = first;

			while(++next != last)
			{
				if(*next < *first)
					return next;

				first = next;
			}
		}

		return last;
	}

	template<typename ForwardIterator, typename Compare>
	ForwardIterator is_sorted_until(ForwardIterator first, ForwardIterator last, Compare compare)
	{
		if(first != last)
		{
			ForwardIterator next = first;

			while(++next != last)
			{
				if(compare(*next, *first))
					return next;

				first = next;
			}
		}

		return last;
	}



	/// merge
	///
	/// This function merges two sorted input sorted ranges into a result sorted range.
	/// This merge is stable in that no element from the first range will be changed
	/// in order relative to other elements from the first range.
	///
	template <typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Compare>
	OutputIterator merge(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, OutputIterator result, Compare compare)
	{
		while((first1 != last1) && (first2 != last2))
		{
			if(compare(*first2, *first1))
			{
				EASTL_VALIDATE_COMPARE(!compare(*first1, *first2)); // Validate that the compare function is sane.
				*result = *first2;
				++first2;
			}
			else
			{
				*result = *first1;
				++first1;
			}
			++result;
		}

		// Check which list is empty and explicitly copy remaining items from the other list.
		// For performance reasons, only a single copy operation is invoked to avoid the potential overhead
		// introduced by chaining two copy operations together.  Even if a copy is of zero size there can
		// be overhead from calling memmove with a zero size copy.
		if (first1 == last1)
		{
			return eastl::copy(first2, last2, result);
		}
		else
		{
			return eastl::copy(first1, last1, result);
		}
	}

	template <typename InputIterator1, typename InputIterator2, typename OutputIterator>
	inline OutputIterator merge(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, OutputIterator result)
	{
		typedef eastl::less<typename eastl::iterator_traits<InputIterator1>::value_type> Less;

		return eastl::merge<InputIterator1, InputIterator2, OutputIterator, Less>
						   (first1, last1, first2, last2, result, Less());
	}


	//////////////////////////////////////////////////////////////////////////////
	/// insertion_sort
	///
	/// insertion_sort is an O(n^2) stable sorting algorithm that starts at the
	/// (k + 1) element and assumes the first (k) elements are sorted.
	/// Then copy_backwards from (k + 1) to the begining any elements where the
	/// (k + 1) element is less than [0, k] elements. The position of k when
	/// (k + 1) element is not less than k is the sorted position of the (k + 1) element.
	///
	/// Example With Intermediate Steps:
	/// (k + 1) == 2 : [3, 2, 1] -> [3, 3, 1] -> [2, 3, 1]
	/// (k + 1) == 1 : [2, 3, 1] -> [2, 3, 3] -> [2, 2, 3] -> [1, 2, 3]
	///              : [1, 2, 3]
	template <typename BidirectionalIterator, typename StrictWeakOrdering>
	void insertion_sort(BidirectionalIterator first, BidirectionalIterator last, StrictWeakOrdering compare)
	{
		typedef typename eastl::iterator_traits<BidirectionalIterator>::value_type value_type;

		if (first != last)
		{
			BidirectionalIterator i = first;

			for (++i; i != last; ++i)
			{
				BidirectionalIterator movePosition = i;

				if (compare(*i, *--movePosition)) // Elements already in place aren't moved.
				{
					EASTL_VALIDATE_COMPARE(!compare(*movePosition, *i));
					value_type insertValue(iter_move(i));
					BidirectionalIterator insertPosition = i;

					if (compare(insertValue, *first)) // If it goes first, move the sorted elements up as one range.
					{
						eastl::move_backward(first, i, ++insertPosition);
						insertPosition = first;
					}
					else
					{
						do
						{
							*insertPosition = iter_move(movePosition);
							insertPosition = movePosition;
						} while (compare(insertValue, *--movePosition)); // *first doesn't follow insertValue, so it stops the scan.
					}

					*insertPosition = eastl::move(insertValue);
				}
			}
		}
	} // insertion_sort


	template <typename BidirectionalIterator>
	void insertion_sort(BidirectionalIterator first, BidirectionalIterator last)
	{
		typedef eastl::less<typename eastl::iterator_traits<BidirectionalIterator>::value_type> Less;

		insertion_sort<BidirectionalIterator>(first, last, Less());

	} // insertion_sort


	namespace Internal
	{
		// Insertion-sorts the elements nSpace apart, all the sequences in one pass over the range: each element is inserted
		// among the sorted elements before it in its sequence. Large gaps leave each insertion about one element to move.
		//
		// A trivially copyable element is taken out into a temporary and the elements before it move up by nSpace: one move
		// per step instead of a swap's three, and the temporary stays in registers.
		template <typename RandomAccessIterator, typename StrictWeakOrdering>
		void shell_sort_insert(RandomAccessIterator first, RandomAccessIterator last, typename eastl::iterator_traits<RandomAccessIterator>::difference_type nSpace, StrictWeakOrdering& compare, true_type)
		{
			typedef typename eastl::iterator_traits<RandomAccessIterator>::value_type value_type;

			for(RandomAccessIterator iInsert = first + nSpace; iInsert != last; ++iInsert)
			{
				if(compare(*iInsert, *(iInsert - nSpace)))
				{
					EASTL_VALIDATE_COMPARE(!compare(*(iInsert - nSpace), *iInsert)); // Validate that the compare function is sane.
					value_type value(iter_move(iInsert));
					RandomAccessIterator iHole = iInsert;

					do
					{
						*iHole = iter_move(iHole - nSpace);
						iHole -= nSpace;
					} while(((iHole - first) >= nSpace) && compare(value, *(iHole - nSpace))); // Don't form an iterator before first.

					*iHole = eastl::move(value);
				}
			}
		}

		// Other elements are swapped down instead: building and destroying a temporary for each insertion costs more than
		// the moves it saves. Through a temporary, 100,000 reversed tuple<int, string> took 4.5 times as long as with swaps,
		// and strings 1.5 times (with MSVC: the moved-from string is reset byte by byte, then read back whole).
		template <typename RandomAccessIterator, typename StrictWeakOrdering>
		void shell_sort_insert(RandomAccessIterator first, RandomAccessIterator last, typename eastl::iterator_traits<RandomAccessIterator>::difference_type nSpace, StrictWeakOrdering& compare, false_type)
		{
			for(RandomAccessIterator iInsert = first + nSpace; iInsert != last; ++iInsert)
			{
				for(RandomAccessIterator iRight = iInsert; compare(*iRight, *(iRight - nSpace)); )
				{
					EASTL_VALIDATE_COMPARE(!compare(*(iRight - nSpace), *iRight)); // Validate that the compare function is sane.
					eastl::iter_swap(iRight, iRight - nSpace);
					iRight -= nSpace;

					if((iRight - first) < nSpace) // Don't form an iterator before first.
						break;
				}
			}
		}
	} // namespace Internal


	/// shell_sort
	///
	/// Implements the ShellSort algorithm. This algorithm is a serious algorithm for larger 
	/// data sets, as reported by Sedgewick in his discussions on QuickSort. Note that shell_sort
	/// requires a random access iterator, which usually means an array (eg. vector, deque).
	/// ShellSort has good performance with presorted sequences.
	/// The term "shell" derives from the name of the inventor, David Shell.
	///
	/// To consider: Allow the user to specify the "h-sequence" array.
	///
	template <typename RandomAccessIterator, typename StrictWeakOrdering>
	void shell_sort(RandomAccessIterator first, RandomAccessIterator last, StrictWeakOrdering compare)
	{
		typedef typename eastl::iterator_traits<RandomAccessIterator>::difference_type difference_type;
		typedef typename eastl::iterator_traits<RandomAccessIterator>::value_type      value_type;

		// We use the Knuth 'h' sequence below, as it is easy to calculate at runtime. 
		// However, possibly we are better off using a different sequence based on a table.
		// One such sequence which averages slightly better than Knuth is:
		//    1, 5, 19, 41, 109, 209, 505, 929, 2161, 3905, 8929, 16001, 36289, 
		//    64769, 146305, 260609, 587521, 1045505, 2354689, 4188161, 9427969, 16764929

		if(first != last)
		{
			const difference_type   nSize  = last - first;
			difference_type         nSpace = 1; // nSpace is the 'h' value of the ShellSort algorithm.

			while(nSpace < nSize)
				nSpace = (nSpace * 3) + 1; // This is the Knuth 'h' sequence: 1, 4, 13, 40, 121, 364, 1093, 3280, 9841, 29524, 88573, 265720, 797161, 2391484, 7174453, 21523360, 64570081, 193710244, 

			// nSpace will iterate from the largest Knuth 'h' element smaller than the size of the range down to 1 (inclusive).
			for(nSpace = (nSpace - 1) / 3; nSpace >= 1; nSpace = (nSpace - 1) / 3)  // Integer division is less than ideal.
				Internal::shell_sort_insert(first, last, nSpace, compare, eastl::is_trivially_copyable<value_type>());
		}
	} // shell_sort

	template <typename RandomAccessIterator>
	inline void shell_sort(RandomAccessIterator first, RandomAccessIterator last)
	{
		typedef eastl::less<typename eastl::iterator_traits<RandomAccessIterator>::value_type> Less;

		eastl::shell_sort<RandomAccessIterator, Less>(first, last, Less());
	}



	/// heap_sort
	///
	/// Implements the HeapSort algorithm. 
	/// Note that heap_sort requires a random access iterator, which usually means 
	/// an array (eg. vector, deque).
	///
	template <typename RandomAccessIterator, typename StrictWeakOrdering>
	void heap_sort(RandomAccessIterator first, RandomAccessIterator last, StrictWeakOrdering compare)
	{
		// We simply call our heap algorithms to do the work for us.
		eastl::make_heap<RandomAccessIterator, StrictWeakOrdering>(first, last, compare);
		eastl::sort_heap<RandomAccessIterator, StrictWeakOrdering>(first, last, compare);
	}

	template <typename RandomAccessIterator>
	inline void heap_sort(RandomAccessIterator first, RandomAccessIterator last)
	{
		typedef eastl::less<typename eastl::iterator_traits<RandomAccessIterator>::value_type> Less;

		eastl::heap_sort<RandomAccessIterator, Less>(first, last, Less());
	}



	namespace Internal
	{
		// The merge sort that merge_sort, stable_sort and merge_sort_buffer use. Like the MSVC STL's std::stable_sort,
		// merge_sort needs a buffer for only half the range: it sorts each half with the buffer, then merges the halves
		// through it. A range is sorted top-down, alternating between the range and the buffer so that each level moves each
		// element once: merge_sort_in_place sorts the two halves of its range into the buffer and merges them back, and
		// merge_sort_move sorts the two halves of its range in place and merges them into the buffer. Short ranges are
		// insertion-sorted. Parts found already sorted stay where they are, and adjacent runs already in order, or
		// entirely in reverse order, are moved without being merged.

		// Whether elements of type T compared with Compare are cheap enough to compare and copy that the comparisons are
		// better not branched on when branches would be mispredicted: arithmetic elements compared with less or greater.
		// The merges of merge_move_runs and the partitions of quick_sort and nth_element then do without branches.
		template <typename T, typename Compare>
		struct is_branchless_partition : false_type {};
		template <typename T>
		struct is_branchless_partition<T, eastl::less<T>> : is_arithmetic<T> {};
		template <typename T>
		struct is_branchless_partition<T, eastl::greater<T>> : is_arithmetic<T> {};
		template <typename T>
		struct is_branchless_partition<T, eastl::less<void>> : is_arithmetic<T> {};
		template <typename T>
		struct is_branchless_partition<T, eastl::greater<void>> : is_arithmetic<T> {};

		// The longest range that is insertion-sorted: 32 elements like the MSVC STL when moves are copies of trivially
		// copyable elements, else 24, where the insertion sort's extra moves cost more than the merging they save.
		template <typename T>
		struct merge_sort_run_length : integral_constant<intptr_t, is_trivially_copyable<T>::value ? 32 : 24> {};

		// Insertion-sorts the range [first, last), whose part [first, next) is sorted and not empty.
		template <typename RandomAccessIterator, typename StrictWeakOrdering>
		void merge_sort_insertion_sort(RandomAccessIterator first, RandomAccessIterator next, RandomAccessIterator last, StrictWeakOrdering& compare)
		{
			typedef typename eastl::iterator_traits<RandomAccessIterator>::value_type value_type;

			if(((next - first) == 1) && (next != last) && compare(*next, *first))
			{
				// Reverse a strictly decreasing prefix instead of inserting its elements one by one.
				do
					++next;
				while((next != last) && compare(*next, *(next - 1)));

				eastl::reverse(first, next);
			}

			for(; next != last; ++next)
			{
				if(compare(*next, *(next - 1)))
				{
					EASTL_VALIDATE_COMPARE(!compare(*(next - 1), *next)); // Validate that the compare function is sane.
					value_type value(iter_move(next));
					RandomAccessIterator hole = next;

					if(compare(value, *first)) // If it goes first, move the sorted elements up as one range.
						hole = eastl::move_backward(first, next, next + 1) - 1;
					else
					{
						do
						{
							*hole = iter_move(hole - 1);
							--hole;
						} while(compare(value, *(hole - 1))); // *first doesn't follow value, so it stops the scan.
					}

					*hole = eastl::move(value);
				}
			}
		}

		// Moves the non-empty range [first, last) to pDest, insertion-sorting it there.
		template <typename RandomAccessIterator, typename T, typename StrictWeakOrdering>
		void merge_sort_insertion_move(RandomAccessIterator first, RandomAccessIterator last, T* pDest, StrictWeakOrdering& compare)
		{
			T* pDestLast = pDest;
			RandomAccessIterator next = first + 1;

			if((next != last) && compare(*next, *first))
			{
				// Move a strictly decreasing prefix in reverse instead of inserting its elements one by one.
				do
					++next;
				while((next != last) && compare(*next, *(next - 1)));

				for(RandomAccessIterator current = next; current != first; ++pDestLast)
					*pDestLast = iter_move(--current);
			}
			else
			{
				*pDestLast = iter_move(first);
				++pDestLast;
			}

			for(; next != last; ++next, ++pDestLast)
			{
				if(compare(*next, *(pDestLast - 1)))
				{
					EASTL_VALIDATE_COMPARE(!compare(*(pDestLast - 1), *next)); // Validate that the compare function is sane.
					T* pHole = pDestLast;

					if(compare(*next, *pDest)) // If it goes first, move the sorted elements up as one range.
						pHole = eastl::move_backward(pDest, pDestLast, pDestLast + 1) - 1;
					else
					{
						do
						{
							*pHole = eastl::move(*(pHole - 1));
							--pHole;
						} while(compare(*next, *(pHole - 1))); // *pDest doesn't follow *next, so it stops the scan.
					}

					*pHole = iter_move(next);
				}
				else
					*pDestLast = iter_move(next);
			}
		}

		// Moves the adjacent non-empty sorted runs [first, mid) and [mid, last), which are neither in order nor in reverse
		// order, merged, to result.
		template <typename InputIterator, typename OutputIterator, typename StrictWeakOrdering>
		OutputIterator merge_move_runs_impl(InputIterator first, InputIterator mid, InputIterator last, OutputIterator result, StrictWeakOrdering& compare, bool&, false_type)
		{
			for(InputIterator next = mid;;)
			{
				if(compare(*next, *first))
				{
					EASTL_VALIDATE_COMPARE(!compare(*first, *next)); // Validate that the compare function is sane.
					*result = iter_move(next);
					++result;
					if(++next == last)
						return eastl::move(first, mid, result);
				}
				else
				{
					*result = iter_move(first);
					++result;
					if(++first == mid)
						return eastl::move(next, last, result);
				}
			}
		}

		// For elements that is_branchless_partition selects, merging without branches on the comparisons pays off where
		// the branches would be mispredicted: where the two runs interleave unpredictably, as on random input. Merges of
		// periodic or partly sorted inputs take elements from one run and the other in patterns that branch predictors
		// follow: one by one, two by two, or in long streaks. So each merge counts the runs of elements it takes from the
		// same side (nRuns) and those of a single element (nSingles), and the next merge goes without branches when both
		// rates are those of random input, between 0.3 and 0.7 (about 0.5 each).

		// With branches, counting the runs.
		template <typename InputIterator, typename OutputIterator, typename StrictWeakOrdering, typename difference_type>
		OutputIterator merge_move_runs_counting(InputIterator first, InputIterator mid, InputIterator last, OutputIterator result, StrictWeakOrdering& compare, difference_type& nRuns, difference_type& nSingles)
		{
			InputIterator next = mid;
			bool bTakeNext = compare(*next, *first);

			for(;;)
			{
				const OutputIterator runStart = result;

				if(bTakeNext)
				{
					do
					{
						EASTL_VALIDATE_COMPARE(!compare(*first, *next)); // Validate that the compare function is sane.
						*result = iter_move(next);
						++result;
						if(++next == last)
							return eastl::move(first, mid, result);
					} while(compare(*next, *first));
				}
				else
				{
					do
					{
						*result = iter_move(first);
						++result;
						if(++first == mid)
							return eastl::move(next, last, result);
					} while(!compare(*next, *first));
				}

				++nRuns;
				nSingles += difference_type((result - runStart) == 1);
				bTakeNext = !bTakeNext;
			}
		}

		// Without branches, from both ends at once, which makes two independent chains of loads that depend on the
		// comparisons before them: the front takes the least element left (the first run's on ties), the back the
		// greatest (the second run's on ties), while each run has at least two elements left, so that both never take the
		// same one. The front counts the runs during the first 32 steps, over the nCounted elements it takes there.
		template <typename InputIterator, typename OutputIterator, typename StrictWeakOrdering, typename difference_type>
		OutputIterator merge_move_runs_branchless(InputIterator first, InputIterator mid, InputIterator last, OutputIterator result, StrictWeakOrdering& compare, difference_type& nCounted, difference_type& nRuns, difference_type& nSingles)
		{
			InputIterator  firstBack  = mid - 1;
			InputIterator  next       = mid;
			InputIterator  nextBack   = last - 1;
			OutputIterator resultBack = result + ((last - first) - 1);
			const OutputIterator resultEnd = resultBack + 1;

			// The counts use bitwise operators and locals: && and the counters behind references made compilers branch on
			// the changes and store the counters at each step.
			bool bTookNext = false, bChangedFront = false;
			int nStepRuns = 0, nStepSingles = 0;
			int i = 0;

			for(; (i < 32) && ((firstBack - first) >= 1) && ((nextBack - next) >= 1); i++)
			{
				const bool bTakeNext = compare(*next, *first);
				*result = eastl::move(bTakeNext ? *next : *first);
				++result;
				next  += difference_type(bTakeNext);
				first += difference_type(!bTakeNext);

				const bool bTakeFirstBack = compare(*nextBack, *firstBack);
				*resultBack = eastl::move(bTakeFirstBack ? *firstBack : *nextBack);
				--resultBack;
				firstBack -= difference_type(bTakeFirstBack);
				nextBack  -= difference_type(!bTakeFirstBack);

				const bool bChangeFront = (bTakeNext != bTookNext);
				nStepRuns    += int(bChangeFront);
				nStepSingles += int(bChangeFront & bChangedFront);
				bTookNext      = bTakeNext;
				bChangedFront  = bChangeFront;
			}

			nCounted  = difference_type(i);
			nRuns    += difference_type(nStepRuns);
			nSingles += difference_type(nStepSingles);

			while(((firstBack - first) >= 1) && ((nextBack - next) >= 1))
			{
				const bool bTakeNext = compare(*next, *first);
				*result = eastl::move(bTakeNext ? *next : *first);
				++result;
				next  += difference_type(bTakeNext);
				first += difference_type(!bTakeNext);

				const bool bTakeFirstBack = compare(*nextBack, *firstBack);
				*resultBack = eastl::move(bTakeFirstBack ? *firstBack : *nextBack);
				--resultBack;
				firstBack -= difference_type(bTakeFirstBack);
				nextBack  -= difference_type(!bTakeFirstBack);
			}

			// The rest, [first, firstBack] and [next, nextBack], at most one element of one of them, fills
			// [result, resultBack].
			while((first <= firstBack) && (next <= nextBack))
			{
				if(compare(*next, *first))
				{
					*result = iter_move(next);
					++next;
				}
				else
				{
					*result = iter_move(first);
					++first;
				}
				++result;
			}

			// Usually a few elements: loops, rather than eastl::move's calls to memmove.
			for(; first <= firstBack; ++first, ++result)
				*result = iter_move(first);
			for(; next <= nextBack; ++next, ++result)
				*result = iter_move(next);

			return resultEnd;
		}

		template <typename InputIterator, typename OutputIterator, typename StrictWeakOrdering>
		OutputIterator merge_move_runs_impl(InputIterator first, InputIterator mid, InputIterator last, OutputIterator result, StrictWeakOrdering& compare, bool& bBranchless, true_type)
		{
			typedef typename eastl::iterator_traits<InputIterator>::difference_type difference_type;

			difference_type nCounted = last - first;
			difference_type nRuns    = 0;
			difference_type nSingles = 0;

			if(bBranchless)
				result = Internal::merge_move_runs_branchless(first, mid, last, result, compare, nCounted, nRuns, nSingles);
			else
				result = Internal::merge_move_runs_counting(first, mid, last, result, compare, nRuns, nSingles);

			const uint64_t nC = uint64_t(nCounted), nR = uint64_t(nRuns), nS = uint64_t(nSingles);

			bBranchless = ((nR * 10) > (nC * 3)) && ((nR * 10) < (nC * 7)) && ((nS * 10) > (nR * 3)) && ((nS * 10) < (nR * 7));
			return result;
		}

		// Moves the adjacent non-empty sorted runs [first, mid) and [mid, last), merged, to result. bBranchless carries
		// the verdict of the merge before it on how to merge (see merge_move_runs_counting).
		template <typename InputIterator, typename OutputIterator, typename StrictWeakOrdering>
		OutputIterator merge_move_runs(InputIterator first, InputIterator mid, InputIterator last, OutputIterator result, StrictWeakOrdering& compare, bool& bBranchless)
		{
			typedef typename eastl::iterator_traits<InputIterator>::value_type value_type;

			if(!compare(*mid, *(mid - 1)))
				return eastl::move(first, last, result);

			if(compare(*(last - 1), *first))
				return eastl::move(first, mid, eastl::move(mid, last, result));

			return Internal::merge_move_runs_impl(first, mid, last, result, compare, bBranchless,
			                                      is_branchless_partition<value_type, typename eastl::remove_cvref<StrictWeakOrdering>::type>());
		}

		// Merges the adjacent non-empty sorted ranges [first, mid) and [mid, last) in place, using pBuffer, which holds
		// at least as many elements as the shorter of the two.
		template <typename RandomAccessIterator, typename T, typename StrictWeakOrdering>
		void merge_halves_with_buffer(RandomAccessIterator first, RandomAccessIterator mid, RandomAccessIterator last, T* pBuffer, StrictWeakOrdering& compare)
		{
			if(!compare(*mid, *(mid - 1)))
				return;

			// The elements of the first half that don't follow any of the second half, and those of the second half
			// that don't precede any of the first half, stay where they are.
			typedef typename eastl::remove_reference<typename eastl::iterator_traits<RandomAccessIterator>::reference>::type element_type;

			first = eastl::upper_bound<RandomAccessIterator, element_type, StrictWeakOrdering&>(first, mid, *mid, compare);
			last  = eastl::lower_bound<RandomAccessIterator, element_type, StrictWeakOrdering&>(mid, last, *(mid - 1), compare);

			if((mid - first) <= (last - mid))
			{
				// Move the first part to the buffer and merge forward.
				T* const pBufferLast = eastl::move(first, mid, pBuffer);

				for(T* pCurrent = pBuffer;;)
				{
					if(compare(*mid, *pCurrent))
					{
						EASTL_VALIDATE_COMPARE(!compare(*pCurrent, *mid)); // Validate that the compare function is sane.
						*first = iter_move(mid);
						++first;
						if(++mid == last)
						{
							eastl::move(pCurrent, pBufferLast, first);
							return;
						}
					}
					else
					{
						*first = eastl::move(*pCurrent);
						++first;
						if(++pCurrent == pBufferLast)
							return; // The rest of the second part is already in place.
					}
				}
			}
			else
			{
				// Move the second part to the buffer and merge backward.
				T* pCurrent = eastl::move(mid, last, pBuffer) - 1;

				for(--mid;;)
				{
					if(compare(*pCurrent, *mid))
					{
						EASTL_VALIDATE_COMPARE(!compare(*mid, *pCurrent)); // Validate that the compare function is sane.
						*--last = iter_move(mid);
						if(mid == first)
						{
							eastl::move_backward(pBuffer, pCurrent + 1, last);
							return;
						}
						--mid;
					}
					else
					{
						*--last = eastl::move(*pCurrent);
						if(pCurrent == pBuffer)
							return; // The rest of the first part is already in place.
						--pCurrent;
					}
				}
			}
		}

		// Starts the lifetime of the n elements of pBuffer: by default-initialization when that does nothing, else by moving
		// the n elements at first into the buffer and back, so that the elements needn't be default-constructible.
		template <typename RandomAccessIterator, typename T, typename difference_type>
		void merge_sort_construct_buffer(RandomAccessIterator, T* pBuffer, difference_type n, true_type)
		{
			eastl::uninitialized_default_construct(pBuffer, pBuffer + n);
		}

		template <typename RandomAccessIterator, typename T, typename difference_type>
		void merge_sort_construct_buffer(RandomAccessIterator first, T* pBuffer, difference_type n, false_type)
		{
			eastl::uninitialized_move(first, first + n, pBuffer);
			eastl::move(pBuffer, pBuffer + n, first);
		}

		template <typename RandomAccessIterator, typename T, typename difference_type, typename StrictWeakOrdering>
		void merge_sort_move(RandomAccessIterator first, RandomAccessIterator last, T* pDest, difference_type nSorted, StrictWeakOrdering& compare, bool& bBranchless);

		// Sorts the non-empty range [first, last), whose first nSorted elements are known to be sorted when nSorted > 0,
		// using pBuffer, which holds at least (last - first) elements. bBranchless is merge_move_runs'.
		template <typename RandomAccessIterator, typename T, typename difference_type, typename StrictWeakOrdering>
		void merge_sort_in_place(RandomAccessIterator first, RandomAccessIterator last, T* pBuffer, difference_type nSorted, StrictWeakOrdering& compare, bool& bBranchless)
		{
			const difference_type nCount = last - first;

			if(nSorted < 1)
				nSorted = eastl::is_sorted_until<RandomAccessIterator, StrictWeakOrdering&>(first, last, compare) - first;

			if(nSorted < nCount)
			{
				if(nCount <= difference_type(merge_sort_run_length<T>::value))
					merge_sort_insertion_sort(first, first + nSorted, last, compare);
				else
				{
					const difference_type nMid = nCount / 2;
					const RandomAccessIterator mid = first + nMid;
					const difference_type nSortedSecond = eastl::is_sorted_until<RandomAccessIterator, StrictWeakOrdering&>(mid, last, compare) - mid;

					if((nSorted >= nMid) || (nSortedSecond == (nCount - nMid)))
					{
						// A half is sorted: sort the other one in place, then merge them through the buffer.
						merge_sort_in_place(first, mid, pBuffer, nSorted, compare, bBranchless);
						merge_sort_in_place(mid, last, pBuffer, nSortedSecond, compare, bBranchless);
						merge_halves_with_buffer(first, mid, last, pBuffer, compare);
					}
					else
					{
						merge_sort_move(first, mid, pBuffer, nSorted, compare, bBranchless);
						merge_sort_move(mid, last, pBuffer + nMid, nSortedSecond, compare, bBranchless);
						merge_move_runs(pBuffer, pBuffer + nMid, pBuffer + nCount, first, compare, bBranchless);
					}
				}
			}
		}

		// Moves the non-empty range [first, last), whose first nSorted elements are known to be sorted when nSorted > 0,
		// sorted, to pDest, which holds at least (last - first) elements.
		template <typename RandomAccessIterator, typename T, typename difference_type, typename StrictWeakOrdering>
		void merge_sort_move(RandomAccessIterator first, RandomAccessIterator last, T* pDest, difference_type nSorted, StrictWeakOrdering& compare, bool& bBranchless)
		{
			const difference_type nCount = last - first;

			if(nSorted < 1)
				nSorted = eastl::is_sorted_until<RandomAccessIterator, StrictWeakOrdering&>(first, last, compare) - first;

			if(nSorted >= nCount)
				eastl::move(first, last, pDest);
			else if(nCount <= difference_type(merge_sort_run_length<T>::value))
				merge_sort_insertion_move(first, last, pDest, compare);
			else
			{
				const difference_type nMid = nCount / 2;

				merge_sort_in_place(first, first + nMid, pDest, nSorted, compare, bBranchless);
				merge_sort_in_place(first + nMid, last, pDest + nMid, difference_type(0), compare, bBranchless);
				merge_move_runs(first, first + nMid, last, pDest, compare, bBranchless);
			}
		}
	}


	/// merge_sort_buffer
	///
	/// Implements the MergeSort algorithm with a user-supplied buffer.
	/// The input buffer must be able to hold a number of items equal to 'last - first'.
	/// Note that merge_sort_buffer requires a random access iterator, which usually means 
	/// an array (eg. vector, deque).
	///
	/// It is merge_sort's algorithm, sorting the whole range with the buffer instead of each half. It adapts to data that
	/// is already partially sorted: if data is completely sorted, then performance is O(n), and even data with
	/// partially sorted regions benefits. The elements left in the buffer are moved-from.
	///
	template <typename RandomAccessIterator, typename T, typename StrictWeakOrdering>
	void merge_sort_buffer(RandomAccessIterator first, RandomAccessIterator last, T* pBuffer, StrictWeakOrdering compare)
	{
		typedef typename eastl::iterator_traits<RandomAccessIterator>::difference_type difference_type;

		if((last - first) > 1)
		{
			bool bBranchless = false;
			Internal::merge_sort_in_place(first, last, pBuffer, difference_type(0), compare, bBranchless);
		}
	}

	template <typename RandomAccessIterator, typename T>
	inline void merge_sort_buffer(RandomAccessIterator first, RandomAccessIterator last, T* pBuffer)
	{
		typedef eastl::less<typename eastl::iterator_traits<RandomAccessIterator>::value_type> Less;

		eastl::merge_sort_buffer<RandomAccessIterator, T, Less>(first, last, pBuffer, Less());
	}



	/// merge_sort
	///
	/// Implements the MergeSort algorithm.
	/// This algorithm allocates memory via the user-supplied allocator, a buffer for half the elements
	/// (rounded up), unless the range is already sorted or short enough to be insertion-sorted. Use
	/// merge_sort_buffer function if you want a version which doesn't allocate memory.
	/// Note that merge_sort requires a random access iterator, which usually means 
	/// an array (eg. vector, deque).
	/// 
	template <typename RandomAccessIterator, typename Allocator, typename StrictWeakOrdering>
	void merge_sort(RandomAccessIterator first, RandomAccessIterator last, Allocator& allocator, StrictWeakOrdering compare)
	{
		typedef typename eastl::iterator_traits<RandomAccessIterator>::difference_type difference_type;
		typedef typename eastl::iterator_traits<RandomAccessIterator>::value_type      value_type;

		const difference_type nCount = last - first;

		if(nCount > 1)
		{
			const difference_type nSorted = eastl::is_sorted_until<RandomAccessIterator, StrictWeakOrdering&>(first, last, compare) - first;

			if(nSorted < nCount)
			{
				if(nCount <= difference_type(Internal::merge_sort_run_length<value_type>::value))
					Internal::merge_sort_insertion_sort(first, first + nSorted, last, compare);
				else
				{
					// Sort each half using the buffer, then merge them through it.
					const difference_type nHalf = nCount - (nCount / 2);
					const RandomAccessIterator mid = first + nHalf;
					value_type* const pBuffer = (value_type*)allocate_memory(allocator, nHalf * sizeof(value_type), EASTL_ALIGN_OF(value_type), 0);

					Internal::merge_sort_construct_buffer(first, pBuffer, nHalf, eastl::is_trivially_default_constructible<value_type>());

					bool bBranchless = false;
					Internal::merge_sort_in_place(first, mid, pBuffer, nSorted, compare, bBranchless);
					Internal::merge_sort_in_place(mid, last, pBuffer, nSorted - nHalf, compare, bBranchless);
					Internal::merge_halves_with_buffer(first, mid, last, pBuffer, compare);

					eastl::destruct(pBuffer, pBuffer + nHalf);
					EASTLFree(allocator, pBuffer, nHalf * sizeof(value_type));
				}
			}
		}
	}

	template <typename RandomAccessIterator, typename Allocator>
	inline void merge_sort(RandomAccessIterator first, RandomAccessIterator last, Allocator& allocator)
	{
		typedef eastl::less<typename eastl::iterator_traits<RandomAccessIterator>::value_type> Less;

		eastl::merge_sort<RandomAccessIterator, Allocator, Less>(first, last, allocator, Less());
	}



	/// partition
	///
	/// Implements the partition algorithm.
	/// Rearranges the elements in the range [first, last), in such a way that all the elements 
	/// for which pred returns true precede all those for which it returns false. The iterator 
	/// returned points to the first element of the second group.
	/// The relative ordering within each group is not necessarily the same as before the call. 
	/// See function stable_partition for a function with a similar behavior and stability in 
	/// the ordering.
	/// 
	/// To do: Implement a version that uses a faster BidirectionalIterator algorithm for the 
	///        case that the iterator range is a bidirectional iterator instead of just an
	///        input iterator (one direction).
	///
	template<typename InputIterator, typename Predicate>
	InputIterator partition(InputIterator begin, InputIterator end, Predicate predicate)
	{
		if(begin != end) 
		{
			while(predicate(*begin))
			{
				if(++begin == end) 
					return begin;
			}

			InputIterator middle = begin;

			while(++middle != end)
			{
				if(predicate(*middle))
				{
					eastl::swap(*begin, *middle);
					++begin;
				}
			}
		}

		return begin;
	}

	/// stable_partition
	///
	/// Performs the same function as @p partition() with the additional
	/// guarantee that the relative ordering of elements in each group is
	/// preserved.
	template <typename ForwardIterator, typename Predicate>
	ForwardIterator stable_partition(ForwardIterator first, ForwardIterator last, Predicate pred)
	{
		first = eastl::find_if_not(first, last, pred);

		if (first == last)
			return first;

		typedef typename iterator_traits<ForwardIterator>::value_type value_type;

		const auto requested_size = eastl::distance(first, last);

		auto allocator = *get_default_allocator(0);
		value_type* const buffer =
		    (value_type*)allocate_memory(allocator, requested_size * sizeof(value_type), EASTL_ALIGN_OF(value_type), 0);
		eastl::uninitialized_value_construct(buffer, buffer + requested_size);

		ForwardIterator result1 = first;
		value_type* result2 = buffer;

		*result2 = iter_move(first);
		++result2;
		++first;
		for (; first != last; ++first)
		{
			if (pred(*first))
			{
				*result1 = iter_move(first);
				++result1;
			}
			else
			{
				*result2 = iter_move(first);
				++result2;
			}
		}

		eastl::move(buffer, result2, result1);

		eastl::destruct(buffer, buffer + requested_size);
		EASTLFree(allocator, buffer, requested_size * sizeof(value_type));
		
		return result1;
	}

	/////////////////////////////////////////////////////////////////////
	// quick_sort
	//
	// We do the "introspection sort" variant of quick sort which is now
	// well-known and understood. You can read about this algorithm in
	// many articles on quick sort, but briefly what it does is a median-
	// of-three quick sort whereby the recursion depth is limited to a
	// some value (after which it gives up on quick sort and switches to
	// a heap sort) and whereby after a certain amount of sorting the 
	// algorithm stops doing quick-sort and finishes the sorting via
	// a simple insertion sort.
	/////////////////////////////////////////////////////////////////////

	#if (defined(EA_PROCESSOR_X86) || defined(EA_PROCESSOR_X86_64))
		static const int kQuickSortLimit = 28; // For sorts of random arrays over 100 items, 28 - 32 have been found to be good numbers on x86.
	#else
		static const int kQuickSortLimit = 16; // It seems that on other processors lower limits are more beneficial, as they result in fewer compares.
	#endif

	namespace Internal
	{
		template <typename Size>
		inline Size Log2(Size n)
		{
			int i;
			for(i = 0; n; ++i)
				n >>= 1;
			return i - 1;
		}

		// To do: Investigate the speed of this bit-trick version of Log2.
		//        It may work better on some platforms but not others.
		//
		// union FloatUnion {
		//     float    f;
		//     uint32_t i;
		// };
		// 
		// inline uint32_t Log2(uint32_t x)
		// {
		//     const FloatInt32Union u = { x };
		//     return (u.i >> 23) - 127;
		// }
	}

	template <typename RandomAccessIterator, typename T>
	inline RandomAccessIterator get_partition_impl(RandomAccessIterator first, RandomAccessIterator last, T&& pivotValue)
	{
		for(; ; ++first)
		{
			while(*first < pivotValue)
			{
				EASTL_VALIDATE_COMPARE(!(pivotValue < *first)); // Validate that the compare function is sane.
				++first;
			}
			--last;

			while(pivotValue < *last)
			{
				EASTL_VALIDATE_COMPARE(!(*last < pivotValue)); // Validate that the compare function is sane.
				--last;
			}

			if(first >= last) // Random access iterators allow operator >=
				return first;

			eastl::iter_swap(first, last);
		}
	}

	/// get_partition
	///
	/// This function takes const T& instead of T because T may have special alignment
	/// requirements and some compilers (e.g. VC++) are don't respect alignment requirements
	/// for function arguments.
	///
	template <typename RandomAccessIterator, typename T>
	inline RandomAccessIterator get_partition(RandomAccessIterator first, RandomAccessIterator last, const T& pivotValue)
	{
		const T pivotCopy(pivotValue); // Need to make a temporary because the sequence below is mutating.
		return get_partition_impl<RandomAccessIterator, const T&>(first, last, pivotCopy);
	}

	template <typename RandomAccessIterator, typename T>
	inline RandomAccessIterator get_partition(RandomAccessIterator first, RandomAccessIterator last, T&& pivotValue)
	{
		// Note: unlike the copy-constructible variant of get_partition... we can't create a temporary const move-constructible object
		return get_partition_impl<RandomAccessIterator, T&&>(first, last, eastl::move(pivotValue));
	}

	template <typename RandomAccessIterator, typename T, typename Compare>
	inline RandomAccessIterator get_partition_impl(RandomAccessIterator first, RandomAccessIterator last, T&& pivotValue, Compare compare)
	{
		for(; ; ++first)
		{
			while(compare(*first, pivotValue))
			{
				EASTL_VALIDATE_COMPARE(!compare(pivotValue, *first)); // Validate that the compare function is sane.
				++first;
			}
			--last;

			while(compare(pivotValue, *last))
			{
				EASTL_VALIDATE_COMPARE(!compare(*last, pivotValue)); // Validate that the compare function is sane.
				--last;
			}

			if(first >= last) // Random access iterators allow operator >=
				return first;

			eastl::iter_swap(first, last);
		}
	}

	template <typename RandomAccessIterator, typename T, typename Compare> 
	inline RandomAccessIterator get_partition(RandomAccessIterator first, RandomAccessIterator last, const T& pivotValue, Compare compare)
	{
		const T pivotCopy(pivotValue); // Need to make a temporary because the sequence below is mutating.
		return get_partition_impl<RandomAccessIterator, const T&, Compare>(first, last, pivotCopy, compare);
	}

	template <typename RandomAccessIterator, typename T, typename Compare>
	inline RandomAccessIterator get_partition(RandomAccessIterator first, RandomAccessIterator last, T&& pivotValue, Compare compare)
	{
		// Note: unlike the copy-constructible variant of get_partition... we can't create a temporary const move-constructible object
		return get_partition_impl<RandomAccessIterator, T&&, Compare>(first, last, eastl::forward<T>(pivotValue), compare);
	}


	template <typename RandomAccessIterator>
	inline void partial_sort(RandomAccessIterator first, RandomAccessIterator middle, RandomAccessIterator last)
	{
		typedef typename eastl::iterator_traits<RandomAccessIterator>::difference_type difference_type;
		typedef typename eastl::iterator_traits<RandomAccessIterator>::value_type      value_type;

		eastl::make_heap<RandomAccessIterator>(first, middle);

		for(RandomAccessIterator i = middle; i < last; ++i)
		{
			if(*i < *first)
			{
				EASTL_VALIDATE_COMPARE(!(*first < *i)); // Validate that the compare function is sane.
				value_type temp(iter_move(i));
				*i = iter_move(first);
				eastl::adjust_heap<RandomAccessIterator, difference_type, value_type>
								  (first, difference_type(0), difference_type(middle - first), difference_type(0), eastl::forward<value_type>(temp));
			}
		}

		eastl::sort_heap<RandomAccessIterator>(first, middle);
	}


	template <typename RandomAccessIterator, typename Compare>
	inline void partial_sort(RandomAccessIterator first, RandomAccessIterator middle, RandomAccessIterator last, Compare compare)
	{
		typedef typename eastl::iterator_traits<RandomAccessIterator>::difference_type difference_type;
		typedef typename eastl::iterator_traits<RandomAccessIterator>::value_type      value_type;

		eastl::make_heap<RandomAccessIterator, Compare>(first, middle, compare);

		for(RandomAccessIterator i = middle; i < last; ++i)
		{
			if(compare(*i, *first))
			{
				EASTL_VALIDATE_COMPARE(!compare(*first, *i)); // Validate that the compare function is sane.
				value_type temp(iter_move(i));
				*i = iter_move(first);
				eastl::adjust_heap<RandomAccessIterator, difference_type, value_type, Compare>
								  (first, difference_type(0), difference_type(middle - first), difference_type(0), eastl::forward<value_type>(temp), compare);
			}
		}

		eastl::sort_heap<RandomAccessIterator, Compare>(first, middle, compare);
	}


	namespace Internal
	{
		// Sorts *a, *b and *c with swaps, which puts their median at b.
		template <typename RandomAccessIterator, typename Compare>
		inline void sort_three(RandomAccessIterator a, RandomAccessIterator b, RandomAccessIterator c, Compare& compare)
		{
			if(compare(*b, *a))
				eastl::iter_swap(a, b);

			if(compare(*c, *b))
			{
				eastl::iter_swap(b, c);

				if(compare(*b, *a))
					eastl::iter_swap(a, b);
			}
		}

		// quick_sort (what sort uses) and nth_element are a pattern-defeating quicksort, after Orson Peters' pdqsort
		// ("Pattern-defeating Quicksort", 2021). It is an introsort that:
		// - takes as the pivot the median of three elements, or above 128 elements the median of three medians of three
		//   (spread across the range for nth_element, which takes a sample near nth instead when nth is in the first or
		//   last quarter of the range);
		// - partitions around the pivot left in its place, then swaps it into its final position (pdq_partition_right);
		// - when the pivot equals the element before the range (a previous pivot, so not greater than any element of the
		//   range), puts the elements equal to it first and leaves them there, since their order is final: ranges with
		//   few distinct values take linear time per value (pdq_partition_left);
		// - after a partition that moved nothing, tries to insertion-sort both parts, giving up after a few moves, so that
		//   sorted and nearly sorted ranges take linear time;
		// - after a very unbalanced partition, swaps a few elements to break the pattern, and after log2(n) of them heap
		//   sorts the rest, so that no input makes it quadratic;
		// - insertion-sorts each part of up to kQuickSortLimit elements as it appears; the element before every part but
		//   the first bounds its scans;
		// - for arithmetic elements compared with less or greater, partitions in blocks of 64 elements without branching
		//   on the comparisons (Edelkamp and Weiss, "BlockQuicksort", 2016), unless a sample suggests that the branches
		//   would be predicted.

		template <typename RandomAccessIterator, typename Compare>
		void pdq_insertion_sort(RandomAccessIterator first, RandomAccessIterator last, Compare& compare)
		{
			typedef typename eastl::iterator_traits<RandomAccessIterator>::value_type value_type;

			if(first == last)
				return;

			for(RandomAccessIterator next = first + 1; next != last; ++next)
			{
				if(compare(*next, *(next - 1)))
				{
					EASTL_VALIDATE_COMPARE(!compare(*(next - 1), *next)); // Validate that the compare function is sane.
					value_type value(iter_move(next));
					RandomAccessIterator hole = next;

					do
					{
						*hole = iter_move(hole - 1);
						--hole;
					} while((hole != first) && compare(value, *(hole - 1)));

					*hole = eastl::move(value);
				}
			}
		}

		// Like pdq_insertion_sort, but the element before first, which isn't greater than any of the range, stops the scans.
		template <typename RandomAccessIterator, typename Compare>
		void pdq_insertion_sort_unguarded(RandomAccessIterator first, RandomAccessIterator last, Compare& compare)
		{
			typedef typename eastl::iterator_traits<RandomAccessIterator>::value_type value_type;

			if(first == last)
				return;

			for(RandomAccessIterator next = first + 1; next != last; ++next)
			{
				if(compare(*next, *(next - 1)))
				{
					EASTL_VALIDATE_COMPARE(!compare(*(next - 1), *next)); // Validate that the compare function is sane.
					value_type value(iter_move(next));
					RandomAccessIterator hole = next;

					do
					{
						*hole = iter_move(hole - 1);
						--hole;
					} while(compare(value, *(hole - 1)));

					*hole = eastl::move(value);
				}
			}
		}

		// Insertion-sorts [first, last) unless that would move more than 8 elements, and returns whether it sorted it.
		template <typename RandomAccessIterator, typename Compare>
		bool pdq_partial_insertion_sort(RandomAccessIterator first, RandomAccessIterator last, Compare& compare)
		{
			typedef typename eastl::iterator_traits<RandomAccessIterator>::difference_type difference_type;
			typedef typename eastl::iterator_traits<RandomAccessIterator>::value_type      value_type;

			if(first == last)
				return true;

			difference_type nMoved = 0;

			for(RandomAccessIterator next = first + 1; next != last; ++next)
			{
				if(compare(*next, *(next - 1)))
				{
					value_type value(iter_move(next));
					RandomAccessIterator hole = next;

					do
					{
						*hole = iter_move(hole - 1);
						--hole;
					} while((hole != first) && compare(value, *(hole - 1)));

					*hole = eastl::move(value);

					nMoved += (next - hole);
					if(nMoved > 8)
						return false;
				}
			}

			return true;
		}

		// Moves the pivot to *first: the median of the first, middle and last elements, or above 128 elements the median
		// of three medians of three. An element not less than the pivot is left in the last three, which bounds the first
		// scan of the partitions.
		template <typename RandomAccessIterator, typename Compare>
		void pdq_choose_pivot(RandomAccessIterator first, RandomAccessIterator last, Compare& compare)
		{
			typedef typename eastl::iterator_traits<RandomAccessIterator>::difference_type difference_type;

			const difference_type nHalf = (last - first) / 2;

			if((last - first) > 128)
			{
				Internal::sort_three(first, first + nHalf, last - 1, compare);
				Internal::sort_three(first + 1, first + (nHalf - 1), last - 2, compare);
				Internal::sort_three(first + 2, first + (nHalf + 1), last - 3, compare);
				Internal::sort_three(first + (nHalf - 1), first + nHalf, first + (nHalf + 1), compare);
				eastl::iter_swap(first, first + nHalf);
			}
			else
				Internal::sort_three(first + nHalf, first, last - 1, compare);
		}

		// Like pdq_choose_pivot, but above 128 elements the samples are spread across the range, every eighth of it, as in
		// the MSVC STL: pdq_select partitions the same range again and again, and on sawtooth inputs the samples next to
		// each other kept giving pivots near the ends (pdq_sort, which partitions both parts, measured faster with them).
		template <typename RandomAccessIterator, typename Compare>
		void pdq_choose_pivot_spread(RandomAccessIterator first, RandomAccessIterator last, Compare& compare)
		{
			typedef typename eastl::iterator_traits<RandomAccessIterator>::difference_type difference_type;

			const RandomAccessIterator middle = first + (last - first) / 2;

			if((last - first) > 128)
			{
				const difference_type nStep = ((last - first) + 1) / 8;

				Internal::sort_three(first, first + nStep, first + 2 * nStep, compare);
				Internal::sort_three(middle - nStep, middle, middle + nStep, compare);
				Internal::sort_three(last - 1 - 2 * nStep, last - 1 - nStep, last - 1, compare);
				Internal::sort_three(first + nStep, middle, last - 1 - nStep, compare);
				eastl::iter_swap(first, middle);
			}
			else
				Internal::sort_three(middle, first, last - 1, compare);
		}

		// pdq_select's pivot when nth is in the first or last quarter of a range of more than 128 elements. The median
		// would leave nth near an end of the part kept, which then only halves at each partition; the sample just past
		// nth's relative position makes that part shrink faster (Floyd and Rivest's selection does the same with two
		// pivots). Of 9 samples, the k-th least is expected at the quantile (k + 1) / 10. They are taken at pseudo-random
		// positions, one in each ninth of the range: at fixed positions, periodic inputs whose period divides their
		// spacing gave samples all alike, and the ends of an organ pipe gave the least ones.
		template <typename RandomAccessIterator, typename Compare>
		void pdq_choose_pivot_rank(RandomAccessIterator first, RandomAccessIterator nth, RandomAccessIterator last, Compare& compare, uint32_t& nRandom)
		{
			typedef typename eastl::iterator_traits<RandomAccessIterator>::difference_type difference_type;

			const difference_type nSize    = last - first;
			const difference_type nSegment = nSize / 9;
			RandomAccessIterator  samples[9];

			for(int i = 0; i < 9; i++)
			{
				// xorshift32 (Marsaglia, "Xorshift RNGs", 2003).
				nRandom ^= nRandom << 13;
				nRandom ^= nRandom >> 17;
				nRandom ^= nRandom << 5;

				const uint64_t nOffset = (uint64_t(nSegment) <= uint64_t(0xFFFFFFFFu)) ? ((uint64_t(nRandom) * uint64_t(nSegment)) >> 32)
				                                                                        : (uint64_t(nRandom) % uint64_t(nSegment));
				samples[i] = first + (difference_type(i) * nSegment + difference_type(nOffset));
			}

			for(int i = 1; i < 9; i++)
			{
				for(int j = i; (j > 0) && compare(*samples[j], *samples[j - 1]); j--)
					eastl::iter_swap(samples[j], samples[j - 1]);
			}

			// The sample of rank 1 to 3 when nth is in the first quarter, 5 to 7 when it is in the last.
			const difference_type nTenth = nSize / 10;
			const difference_type nAt    = nth - first;
			const difference_type nToEnd = (last - 1) - nth;
			int k;

			if(nAt < (nSize / 4))
				k = 1 + int(nAt >= nTenth) + int(nAt >= (2 * nTenth));
			else
				k = 7 - int(nToEnd >= nTenth) - int(nToEnd >= (2 * nTenth));

			eastl::iter_swap(first, samples[k]);
		}

		// Partitions [first, last) around the pivot *first, which stays there during the scans, so that it is never
		// copied, and is then swapped into its final position, which is returned: the elements before it are less than
		// the pivot, the others not less. bAlreadyPartitioned tells whether no element had to be swapped.
		template <typename RandomAccessIterator, typename Compare>
		RandomAccessIterator pdq_partition_right(RandomAccessIterator first, RandomAccessIterator last, Compare& compare, bool& bAlreadyPartitioned, bool /*bWhole*/, false_type)
		{
			RandomAccessIterator lower = first;
			RandomAccessIterator upper = last;

			while(compare(*++lower, *first))
			{
				EASTL_VALIDATE_COMPARE(!compare(*first, *lower)); // Validate that the compare function is sane.
			}

			if((lower - 1) == first)
			{
				while((lower < upper) && !compare(*--upper, *first))
					{}
			}
			else
			{
				while(!compare(*--upper, *first))
					{}
			}

			bAlreadyPartitioned = !(lower < upper);

			while(lower < upper)
			{
				eastl::iter_swap(lower, upper);

				while(compare(*++lower, *first))
				{
					EASTL_VALIDATE_COMPARE(!compare(*first, *lower)); // Validate that the compare function is sane.
				}

				while(!compare(*--upper, *first))
					{}
			}

			const RandomAccessIterator pivotPosition = lower - 1;

			if(pivotPosition != first)
				eastl::iter_swap(first, pivotPosition);

			return pivotPosition;
		}

		// Like the other pdq_partition_right, for arithmetic elements compared with less or greater: a copy of the pivot
		// costs nothing and nothing can observe it, and the comparisons are cheap enough to classify the elements without
		// branches, a block of up to 64 at each end at a time, recording the offsets of those on the wrong side, which are
		// then swapped in pairs.
		//
		// That pays off when the other partition's branches would be mispredicted, which is the case for random input,
		// while those of sorted, reversed, sawtooth and similar inputs are predicted. So a range of 1024 elements or more
		// first has the order of 33 adjacent elements at each end sampled, and so does the whole range of a sort or
		// nth_element (bWhole) from 256 elements on, with n / 32 + 1 of them below 1024: along random input, rising and
		// falling pairs change about two times out of three, and along those inputs far less often. Below 22 changes out of
		// 62 (in proportion for the smaller samples), the range is partitioned with branches. (Without the sample below 1024
		// elements, nth_element on 1000 reversed ints took up to 1.5 times as long as EASTL's previous introselect; sampling
		// the parts of 256 to 1023 elements of larger ranges too made sawtooth input of 10,000 to 1M elements up to 1.12
		// times slower.) The sample leaves out the pivot: compared with a pivot far from the median, which isn't rare in the
		// parts of a random range, random elements change between "less" and "not less" rarely too.
		template <typename RandomAccessIterator, typename Compare>
		RandomAccessIterator pdq_partition_right(RandomAccessIterator first, RandomAccessIterator last, Compare& compare, bool& bAlreadyPartitioned, bool bWhole, true_type)
		{
			typedef typename eastl::iterator_traits<RandomAccessIterator>::value_type value_type;

			if((last - first) >= (bWhole ? 256 : 1024))
			{
				const int nPairs = ((last - first) >= 1024) ? 32 : int((last - first) / 32);
				int nChanges = 0;
				bool bPrevious = compare(*(first + 2), *(first + 1));

				for(int i = 2; i <= nPairs; i++)
				{
					const bool bFalling = compare(*(first + (i + 1)), *(first + i));
					nChanges += int(bFalling != bPrevious);
					bPrevious = bFalling;
				}

				bPrevious = compare(*(last - 2), *(last - 1));

				for(int i = 2; i <= nPairs; i++)
				{
					const bool bFalling = compare(*(last - (i + 1)), *(last - i));
					nChanges += int(bFalling != bPrevious);
					bPrevious = bFalling;
				}

				if((nChanges * 31) < (22 * (nPairs - 1)))
					return Internal::pdq_partition_right(first, last, compare, bAlreadyPartitioned, bWhole, false_type());
			}

			const value_type pivot(*first);
			RandomAccessIterator lower = first;
			RandomAccessIterator upper = last;

			while(compare(*++lower, pivot))
				{}

			if((lower - 1) == first)
			{
				while((lower < upper) && !compare(*--upper, pivot))
					{}
			}
			else
			{
				while(!compare(*--upper, pivot))
					{}
			}

			bAlreadyPartitioned = !(lower < upper);

			if(!bAlreadyPartitioned)
			{
				eastl::iter_swap(lower, upper);
				++lower;

				const size_t kBlockSize = 64;
				alignas(64) unsigned char offsetsLeft[kBlockSize];
				alignas(64) unsigned char offsetsRight[kBlockSize];
				RandomAccessIterator baseLeft = lower;
				RandomAccessIterator baseRight = upper;
				size_t nLeft = 0, nRight = 0, nStartLeft = 0, nStartRight = 0;

				while(lower < upper)
				{
					// Fill a block of offsets at each end whose last block has been used up, splitting the rest when
					// fewer than two blocks remain.
					const size_t nUnknown    = (size_t)(upper - lower);
					const size_t nSplitLeft  = (nLeft == 0) ? ((nRight == 0) ? (nUnknown / 2) : nUnknown) : 0;
					const size_t nSplitRight = (nRight == 0) ? (nUnknown - nSplitLeft) : 0;

					if(nSplitLeft >= kBlockSize)
					{
						for(size_t i = 0; i < kBlockSize; )
						{
							for(int j = 0; j < 8; j++)
							{
								offsetsLeft[nLeft] = (unsigned char)i++;
								nLeft += size_t(!compare(*lower, pivot));
								++lower;
							}
						}
					}
					else
					{
						for(size_t i = 0; i < nSplitLeft; )
						{
							offsetsLeft[nLeft] = (unsigned char)i++;
							nLeft += size_t(!compare(*lower, pivot));
							++lower;
						}
					}

					if(nSplitRight >= kBlockSize)
					{
						for(size_t i = 0; i < kBlockSize; )
						{
							for(int j = 0; j < 8; j++)
							{
								offsetsRight[nRight] = (unsigned char)++i;
								nRight += size_t(compare(*--upper, pivot));
							}
						}
					}
					else
					{
						for(size_t i = 0; i < nSplitRight; )
						{
							offsetsRight[nRight] = (unsigned char)++i;
							nRight += size_t(compare(*--upper, pivot));
						}
					}

					// Exchange the misplaced elements found at both ends: with swaps when there are as many on each
					// side, else as one cycle, which moves each element once.
					const size_t nCount = (nLeft < nRight) ? nLeft : nRight;

					if(nLeft == nRight)
					{
						for(size_t i = 0; i < nCount; i++)
							eastl::iter_swap(baseLeft + offsetsLeft[nStartLeft + i], baseRight - offsetsRight[nStartRight + i]);
					}
					else if(nCount > 0)
					{
						RandomAccessIterator l = baseLeft + offsetsLeft[nStartLeft];
						RandomAccessIterator r = baseRight - offsetsRight[nStartRight];
						const value_type temp(*l);
						*l = *r;

						for(size_t i = 1; i < nCount; i++)
						{
							l = baseLeft + offsetsLeft[nStartLeft + i];
							*r = *l;
							r = baseRight - offsetsRight[nStartRight + i];
							*l = *r;
						}

						*r = temp;
					}

					nLeft -= nCount;
					nRight -= nCount;
					nStartLeft += nCount;
					nStartRight += nCount;

					if(nLeft == 0)
					{
						nStartLeft = 0;
						baseLeft = lower;
					}

					if(nRight == 0)
					{
						nStartRight = 0;
						baseRight = upper;
					}
				}

				// One side may have misplaced elements left: swap them to the middle.
				if(nLeft != 0)
				{
					while(nLeft-- != 0)
						eastl::iter_swap(baseLeft + offsetsLeft[nStartLeft + nLeft], --upper);
					lower = upper;
				}

				if(nRight != 0)
				{
					while(nRight-- != 0)
					{
						eastl::iter_swap(baseRight - offsetsRight[nStartRight + nRight], lower);
						++lower;
					}
				}
			}

			const RandomAccessIterator pivotPosition = lower - 1;
			*first = *pivotPosition;
			*pivotPosition = pivot;
			return pivotPosition;
		}

		// Partitions [first, last) around the pivot *first into the elements not greater than it, first, and the others,
		// and returns the position of the last of the former, where the pivot is swapped. It is used when the element
		// before the range equals the pivot: no element is then less than it, and the former all equal it.
		template <typename RandomAccessIterator, typename Compare>
		RandomAccessIterator pdq_partition_left(RandomAccessIterator first, RandomAccessIterator last, Compare& compare)
		{
			RandomAccessIterator lower = first;
			RandomAccessIterator upper = last;

			while(compare(*first, *--upper)) // The pivot itself stops it.
				{}

			if((upper + 1) == last)
			{
				while((lower < upper) && !compare(*first, *++lower))
					{}
			}
			else
			{
				while(!compare(*first, *++lower))
					{}
			}

			while(lower < upper)
			{
				eastl::iter_swap(lower, upper);

				while(compare(*first, *--upper))
					{}

				while(!compare(*first, *++lower))
					{}
			}

			if(upper != first)
				eastl::iter_swap(first, upper);

			return upper;
		}

		// After a very unbalanced partition, swaps elements around the quarters of each part that isn't short, so that
		// the next pivots don't follow the same pattern.
		template <typename RandomAccessIterator, typename difference_type>
		void pdq_break_patterns(RandomAccessIterator first, RandomAccessIterator pivotPosition, RandomAccessIterator last, difference_type nSizeLeft, difference_type nSizeRight)
		{
			if(nSizeLeft >= difference_type(kQuickSortLimit))
			{
				eastl::iter_swap(first, first + nSizeLeft / 4);
				eastl::iter_swap(pivotPosition - 1, pivotPosition - nSizeLeft / 4);

				if(nSizeLeft > 128)
				{
					eastl::iter_swap(first + 1, first + (nSizeLeft / 4 + 1));
					eastl::iter_swap(first + 2, first + (nSizeLeft / 4 + 2));
					eastl::iter_swap(pivotPosition - 2, pivotPosition - (nSizeLeft / 4 + 1));
					eastl::iter_swap(pivotPosition - 3, pivotPosition - (nSizeLeft / 4 + 2));
				}
			}

			if(nSizeRight >= difference_type(kQuickSortLimit))
			{
				eastl::iter_swap(pivotPosition + 1, pivotPosition + (1 + nSizeRight / 4));
				eastl::iter_swap(last - 1, last - nSizeRight / 4);

				if(nSizeRight > 128)
				{
					eastl::iter_swap(pivotPosition + 2, pivotPosition + (2 + nSizeRight / 4));
					eastl::iter_swap(pivotPosition + 3, pivotPosition + (3 + nSizeRight / 4));
					eastl::iter_swap(last - 2, last - (1 + nSizeRight / 4));
					eastl::iter_swap(last - 3, last - (2 + nSizeRight / 4));
				}
			}
		}

		EA_DISABLE_VC_WARNING(4702) // unreachable code (MSVC reports it for comparators that would make a scan endless)
		template <typename RandomAccessIterator, typename Compare, typename Branchless>
		void pdq_sort(RandomAccessIterator first, RandomAccessIterator last, Compare& compare, int nBadAllowed, bool bLeftmost, bool bWhole, Branchless branchless)
		{
			typedef typename eastl::iterator_traits<RandomAccessIterator>::difference_type difference_type;

			for(;;)
			{
				const difference_type nSize = last - first;

				if(nSize <= difference_type(kQuickSortLimit))
				{
					if(bLeftmost)
						Internal::pdq_insertion_sort(first, last, compare);
					else
						Internal::pdq_insertion_sort_unguarded(first, last, compare);
					return;
				}

				Internal::pdq_choose_pivot(first, last, compare);

				if(!bLeftmost && !compare(*(first - 1), *first))
				{
					first = Internal::pdq_partition_left(first, last, compare) + 1;
					continue;
				}

				bool bAlreadyPartitioned;
				const RandomAccessIterator pivotPosition = Internal::pdq_partition_right(first, last, compare, bAlreadyPartitioned, bWhole, branchless);
				const difference_type nSizeLeft  = pivotPosition - first;
				const difference_type nSizeRight = last - (pivotPosition + 1);
				bWhole = false;

				if((nSizeLeft < (nSize / 8)) || (nSizeRight < (nSize / 8)))
				{
					if(--nBadAllowed == 0)
					{
						eastl::partial_sort<RandomAccessIterator, Compare&>(first, last, last, compare);
						return;
					}

					Internal::pdq_break_patterns(first, pivotPosition, last, nSizeLeft, nSizeRight);
				}
				else if(bAlreadyPartitioned && Internal::pdq_partial_insertion_sort(first, pivotPosition, compare) &&
				        Internal::pdq_partial_insertion_sort(pivotPosition + 1, last, compare))
					return;

				Internal::pdq_sort(first, pivotPosition, compare, nBadAllowed, bLeftmost, false, branchless);
				first = pivotPosition + 1;
				bLeftmost = false;
			}
		}

		// nth_element's selection: the same partitions, continuing with the part that holds nth. It stops when nth is the
		// pivot or one of the elements equal to the previous pivot. The pivot is the spread median of medians, or, when nth
		// is in the first or last quarter, a sample near nth (pdq_choose_pivot_rank). A partition that keeps more than 7/8
		// of the range breaks the pattern as in pdq_sort, and after log2(n) of them it selects with a heap (introselect),
		// so that it is O(n log n) in the worst case.
		template <typename RandomAccessIterator, typename Compare, typename Branchless>
		void pdq_select(RandomAccessIterator first, RandomAccessIterator nth, RandomAccessIterator last, Compare& compare, Branchless branchless)
		{
			typedef typename eastl::iterator_traits<RandomAccessIterator>::difference_type difference_type;

			bool bLeftmost = true;
			bool bWhole = true;
			int nBadAllowed = int(Internal::Log2(last - first));
			uint32_t nRandom = uint32_t(last - first) | 1u; // Not 0, which xorshift32 would keep.

			while((last - first) > difference_type(kQuickSortLimit))
			{
				const difference_type nSize = last - first;
				const difference_type nAt   = nth - first;

				if((nSize > 128) && ((nAt < (nSize / 4)) || (nAt >= (nSize - nSize / 4))))
					Internal::pdq_choose_pivot_rank(first, nth, last, compare, nRandom);
				else
					Internal::pdq_choose_pivot_spread(first, last, compare);

				if(!bLeftmost && !compare(*(first - 1), *first))
				{
					const RandomAccessIterator lastEqual = Internal::pdq_partition_left(first, last, compare);

					if(nth <= lastEqual)
						return;

					first = lastEqual + 1;
					continue;
				}

				bool bAlreadyPartitioned;
				const RandomAccessIterator pivotPosition = Internal::pdq_partition_right(first, last, compare, bAlreadyPartitioned, bWhole, branchless);
				bWhole = false;

				if(nth == pivotPosition)
					return;

				const difference_type nSizeLeft  = pivotPosition - first;
				const difference_type nSizeRight = last - (pivotPosition + 1);
				const difference_type nSizeKept  = (nth < pivotPosition) ? nSizeLeft : nSizeRight;

				if(nSizeKept > (nSize - nSize / 8))
				{
					if(--nBadAllowed == 0)
					{
						eastl::partial_sort<RandomAccessIterator, Compare&>(first, nth + 1, last, compare);
						return;
					}

					Internal::pdq_break_patterns(first, pivotPosition, last, nSizeLeft, nSizeRight);
				}

				if(nth < pivotPosition)
					last = pivotPosition;
				else
				{
					first = pivotPosition + 1;
					bLeftmost = false;
				}
			}

			if(bLeftmost)
				Internal::pdq_insertion_sort(first, last, compare);
			else
				Internal::pdq_insertion_sort_unguarded(first, last, compare);
		}
		EA_RESTORE_VC_WARNING()
	}


	/// nth_element
	///
	/// Rearranges [first, last) so that *nth is the element that would be there if the range were sorted, with no
	/// element before it greater and none after it less. The range is partitioned the way quick_sort does it, only
	/// continuing with the part that holds nth, and after log2(n) partitions that keep most of the range it selects with
	/// a heap (introselect), so that it is O(n log n) in the worst case.
	///
	template<typename RandomAccessIterator, typename Compare>
	inline void nth_element(RandomAccessIterator first, RandomAccessIterator nth, RandomAccessIterator last, Compare compare)
	{
		typedef typename eastl::iterator_traits<RandomAccessIterator>::value_type value_type;

		if(nth != last)
			Internal::pdq_select(first, nth, last, compare, Internal::is_branchless_partition<value_type, typename eastl::remove_cvref<Compare>::type>());
	}


	template<typename RandomAccessIterator>
	inline void nth_element(RandomAccessIterator first, RandomAccessIterator nth, RandomAccessIterator last)
	{
		eastl::nth_element<RandomAccessIterator, eastl::less<>>(first, nth, last, eastl::less<>());
	}


	/// quick_sort
	///
	/// This is an unstable sort.
	/// quick_sort sorts the elements in [first, last) into ascending order, 
	/// meaning that if i and j are any two valid iterators in [first, last) 
	/// such that i precedes j, then *j is not less than *i. quick_sort is not 
	/// guaranteed to be stable. That is, suppose that *i and *j are equivalent: 
	/// neither one is less than the other. It is not guaranteed that the 
	/// relative order of these two elements will be preserved by sort.
	///
	/// It is a pattern-defeating quicksort (see Internal::pdq_sort): an introsort that also takes linear time on sorted
	/// and nearly sorted ranges and on ranges with few distinct values, and partitions arithmetic elements compared with
	/// less or greater without branches.
	///
	template <typename RandomAccessIterator, typename Compare>
	void quick_sort(RandomAccessIterator first, RandomAccessIterator last, Compare compare)
	{
		typedef typename eastl::iterator_traits<RandomAccessIterator>::value_type value_type;

		if((last - first) > 1)
			Internal::pdq_sort(first, last, compare, int(Internal::Log2(last - first)), true, true,
				Internal::is_branchless_partition<value_type, typename eastl::remove_cvref<Compare>::type>());
	}


	template <typename RandomAccessIterator>
	void quick_sort(RandomAccessIterator first, RandomAccessIterator last)
	{
		eastl::quick_sort<RandomAccessIterator, eastl::less<>>(first, last, eastl::less<>());
	}




	namespace Internal
	{
		// Portions of the tim_sort code were originally written by Christopher Swenson.
		// https://github.com/swenson/sort
		// All code in this repository, unless otherwise specified, is hereby licensed under the 
		// MIT Public License: Copyright (c) 2010 Christopher Swenson

		const intptr_t kTimSortStackSize = 64; // Question: What's the upper-limit size requirement for this?

		struct tim_sort_run
		{
			intptr_t start;
			intptr_t length;
		};


		// reverse_elements
		//
		// Reverses the range [first + start, first + start + size)
		// To consider: Use void eastl::reverse(BidirectionalIterator first, BidirectionalIterator last);
		//
		template <typename RandomAccessIterator>
		void reverse_elements(RandomAccessIterator first, intptr_t start, intptr_t end)
		{
			while(start < end)
			{
				eastl::iter_swap(first + start, first + end);
				++start;
				--end;
			}
		}


		// tim_sort_count_run
		//
		// Finds the length of a run which is already sorted (either up or down).
		// If the run is in reverse order, this function puts it in regular order.
		//
		template <typename RandomAccessIterator, typename StrictWeakOrdering>
		intptr_t tim_sort_count_run(const RandomAccessIterator first, const intptr_t start, const intptr_t size, StrictWeakOrdering compare)
		{
			if((size - start) > 1) // If there is anything in the set...
			{
				intptr_t curr = (start + 2);
				
				if(!compare(*(first + start + 1), *(first + start))) // If (first[start + 1] >= first[start]) (If the run is increasing) ...
				{
					for(;; ++curr)
					{
						if(curr >= (size - 1)) // If we are at the end of the data... this run is done.
							break;

						if(compare(*(first + curr), *(first + curr - 1))) // If this item is not in order... this run is done.
							break;
					}
				}
				else  // Else it is decreasing.
				{
					for(;; ++curr)
					{
						if(curr >= (size - 1))  // If we are at the end of the data... this run is done.
							break;

						if(!compare(*(first + curr), *(first + curr - 1)))  // If this item is not in order... this run is done.
							break;                                          // Note that we intentionally compare against <= 0 and not just < 0. This is because 
					}                                                       // The reverse_elements call below could reverse two equal elements and break our stability requirement.

					reverse_elements(first, start, curr - 1);
				}

				return (curr - start);
			}

			// Else we have just one item in the set.       
			return 1;
		}


		// Input   Return
		// --------------
		//  64      32
		//  65      33
		//  66      33
		//  67      34
		//  68      34
		// ...
		// 125      63
		// 126      63
		// 127      64
		// 128      32
		// 129      33
		// 130      33
		// 131      33
		// 132      33
		// 133      34
		// 134      34
		// 135      34
		// 136      34
		// 137      35
		// ...
		//
		// This function will return a value that is always in the range of [32, 64].
		//
		static inline intptr_t timsort_compute_minrun(intptr_t size)
		{
			const int32_t  top_bit = (int32_t)((sizeof(intptr_t) * 8) - countl_zero((uintptr_t)size));
			const int32_t  shift   = (top_bit > 6) ? (top_bit - 6) : 0;
			const intptr_t mask    = (intptr_t(1) << shift) - 1;
				  intptr_t minrun  = (intptr_t)(size >> shift);

			if(mask & size)
				++minrun;

			return minrun;
		}


		template <typename RandomAccessIterator, typename T, typename StrictWeakOrdering>
		void tim_sort_merge(RandomAccessIterator first, const tim_sort_run* run_stack, const intptr_t stack_curr, 
							T* pBuffer, StrictWeakOrdering compare)
		{
			const intptr_t A    = run_stack[stack_curr - 2].length;
			const intptr_t B    = run_stack[stack_curr - 1].length;
			const intptr_t curr = run_stack[stack_curr - 2].start;

			EASTL_DEV_ASSERT((A < 10000000) && (B < 10000000) && (curr < 10000000)); // Sanity check.

			// The buffer holds at least as many elements as the shorter run.
			merge_halves_with_buffer(first + curr, first + curr + A, first + curr + A + B, pBuffer, compare);
		}


		// See the timsort.txt file for an explanation of this function.
		//
		// ------------------------------------------------------------------------
		// What turned out to be a good compromise maintains two invariants on the
		// stack entries, where A, B and C are the lengths of the three righmost 
		// not-yet merged slices:
		//    1.  A > B+C
		//    2.  B > C
		// ------------------------------------------------------------------------
		//
		static inline bool timsort_check_invariant(tim_sort_run* run_stack, const intptr_t stack_curr)
		{
			// To do: Optimize this for the most common type of values.
			if(stack_curr > 2)
			{
				const intptr_t A = run_stack[stack_curr - 3].length;
				const intptr_t B = run_stack[stack_curr - 2].length;
				const intptr_t C = run_stack[stack_curr - 1].length;

				EASTL_DEV_ASSERT((A < 10000000) && (B < 10000000) && (C < 10000000)); // Sanity check.

				if((A <= (B + C)) || (B <= C))
					return true; // Merge the right-most runs.
			}
			else if(stack_curr == 2)
			{
				const intptr_t A = run_stack[stack_curr - 2].length;
				const intptr_t B = run_stack[stack_curr - 1].length;

				EASTL_DEV_ASSERT((A < 10000000) && (B < 10000000)); // Sanity check.

				if(A <= B)
					return true; // Merge the right-most runs.
			}

			return false; // Don't merge the right-most runs.
		}


		template <typename RandomAccessIterator, typename T, typename StrictWeakOrdering>
		intptr_t tim_sort_collapse(RandomAccessIterator first, tim_sort_run* run_stack, intptr_t stack_curr, 
								   T* pBuffer, const intptr_t size, StrictWeakOrdering compare)
		{
			// If the run_stack only has one thing on it, we are done with the collapse.
			while(stack_curr > 1)
			{
				// If this is the last merge, just do it.
				if((stack_curr == 2) && ((run_stack[0].length + run_stack[1].length) == size))
				{
					tim_sort_merge<RandomAccessIterator, T, StrictWeakOrdering>(first, run_stack, stack_curr, pBuffer, compare);
					run_stack[0].length += run_stack[1].length;
					stack_curr--;

					#if EASTL_DEV_DEBUG
						memset(&run_stack[stack_curr], 0, sizeof(run_stack[stack_curr]));
					#endif

					break;
				}
				// Check if the invariant is off for a run_stack of 2 elements.
				else if((stack_curr == 2) && (run_stack[0].length <= run_stack[1].length))
				{
					tim_sort_merge<RandomAccessIterator, T, StrictWeakOrdering>(first, run_stack, stack_curr, pBuffer, compare);
					run_stack[0].length += run_stack[1].length;
					stack_curr--;

					#if EASTL_DEV_DEBUG
						memset(&run_stack[stack_curr], 0, sizeof(run_stack[stack_curr]));
					#endif

					break;
				}
				else if (stack_curr == 2)
					break;

				const intptr_t A = run_stack[stack_curr - 3].length;
				const intptr_t B = run_stack[stack_curr - 2].length;
				const intptr_t C = run_stack[stack_curr - 1].length;
				
				if(A <= (B + C)) // Check first invariant.
				{
					if(A < C)
					{
						tim_sort_merge<RandomAccessIterator, T, StrictWeakOrdering>(first, run_stack, stack_curr - 1, pBuffer, compare);

						stack_curr--;
						run_stack[stack_curr - 2].length += run_stack[stack_curr - 1].length;   // Merge A and B.
						run_stack[stack_curr - 1] = run_stack[stack_curr];

						#if EASTL_DEV_DEBUG
							EASTL_DEV_ASSERT((run_stack[stack_curr - 2].start + run_stack[stack_curr - 2].length) <= size);
							EASTL_DEV_ASSERT((run_stack[stack_curr - 1].start + run_stack[stack_curr - 1].length) <= size);
							memset(&run_stack[stack_curr], 0, sizeof(run_stack[stack_curr]));
						#endif
					}
					else
					{
						tim_sort_merge<RandomAccessIterator, T, StrictWeakOrdering>(first, run_stack, stack_curr, pBuffer, compare);                  // Merge B and C.

						stack_curr--;
						run_stack[stack_curr - 1].length += run_stack[stack_curr].length;

						#if EASTL_DEV_DEBUG
							EASTL_DEV_ASSERT((run_stack[stack_curr - 1].start + run_stack[stack_curr - 1].length) <= size);
							memset(&run_stack[stack_curr], 0, sizeof(run_stack[stack_curr]));
						#endif
					}
				}
				else if(B <= C) // Check second invariant
				{
					tim_sort_merge<RandomAccessIterator, T, StrictWeakOrdering>(first, run_stack, stack_curr, pBuffer, compare);

					stack_curr--;
					run_stack[stack_curr - 1].length += run_stack[stack_curr].length;       // Merge B and C.

					#if EASTL_DEV_DEBUG
						EASTL_DEV_ASSERT((run_stack[stack_curr - 1].start + run_stack[stack_curr - 1].length) <= size);
						memset(&run_stack[stack_curr], 0, sizeof(run_stack[stack_curr]));
					#endif
				}
				else
					break;
			}

			return stack_curr;
		}


		// tim_sort_add_run
		//
		// Return true if the sort is done.
		//
		template <typename RandomAccessIterator, typename T, typename StrictWeakOrdering>
		bool tim_sort_add_run(tim_sort_run* run_stack, RandomAccessIterator first, T* pBuffer, const intptr_t size, const intptr_t minrun, 
							  intptr_t& len, intptr_t& run, intptr_t& curr, intptr_t& stack_curr, StrictWeakOrdering compare)
		{
			len = tim_sort_count_run<RandomAccessIterator, StrictWeakOrdering>(first, curr, size, compare); // This will count the length of the run and reverse the run if it is backwards.
			run = minrun;

			if(run < minrun)            // Always make runs be of minrun length (we'll sort the additional data as needed below)
			   run = minrun;

			if(run > (size - curr))     // But if there isn't minrun data remaining, just sort what's remaining.
			   run = (size - curr);

			if(run > len)               // If there is any additional data we want to sort to bring up the run length to minrun.
			{
				merge_sort_insertion_sort(first + curr, first + curr + len, first + curr + run, compare);
				len = run;
			}

			// At this point, run will be equal to minrun or will go to the end of our data.
			// Add this run to our stack of runs.
			EASTL_DEV_ASSERT(stack_curr < kTimSortStackSize);
			EASTL_DEV_ASSERT((curr >= 0) && (curr < size) && ((curr + len) <= size));

			run_stack[stack_curr].start  = curr;
			run_stack[stack_curr].length = len;
			stack_curr++;

			// Move to the beginning of the next run in the data.
			curr += len;

			if(curr == size)    // If we have hit the end of the data...
			{
				while(stack_curr > 1) // If there is any more than one run... (else all the data is sorted)
				{
					tim_sort_merge<RandomAccessIterator, T, StrictWeakOrdering>(first, run_stack, stack_curr, pBuffer, compare);

					run_stack[stack_curr - 2].length += run_stack[stack_curr - 1].length;
					stack_curr--;

					#if EASTL_DEV_DEBUG
						EASTL_DEV_ASSERT((run_stack[stack_curr - 1].start + run_stack[stack_curr - 1].length) <= size);
						memset(&run_stack[stack_curr], 0, sizeof(run_stack[stack_curr]));
					#endif
				}

				return true; // We are done with sorting.
			}

			return false;
		}

	} // namespace Internal


	// tim_sort_buffer
	//
	/// This is a stable sort.
	// Implements the tim-sort sorting algorithm with a user-provided scratch buffer.
	// http://en.wikipedia.org/wiki/Timsort
	// This sort is the fastest sort when sort stability (maintaining order of equal values) is required and
	// data sets are non-trivial (size >= 15). It's also the fastest sort (e.g. faster than quick_sort) for 
	// the case that at at least half your data is already sorted. Otherwise, eastl::quick_sort is about 10% 
	// faster than tim_sort_buffer but is not a stable sort. There are some reports that tim_sort outperforms
	// quick_sort but most of these aren't taking into account that optimal quick_sort implementations use
	// a hybrid approach called "introsort" (http://en.wikipedia.org/wiki/Introsort) which improves quick_sort
	// considerably in practice.
	//
	// Strengths:
	//     - Fastest stable sort for most sizes of data.
	//     - Fastest sort for containers of data already mostly sorted.
	//     - Simpler to understand than quick_sort.
	//
	// Weaknesses:
	//     - User must provide a scratch buffer, otherwise the buffer is dynamically allocated during runtime.
	//     - Not as fast as quick_sort for the general case of randomized data.
	//     - Requires a RandomAccessIterator; thus must be on an array container type and not a list container type.
	//     - Uses a lot of code to implement; thus it's not great when there is little room for more code.
	//
	// The pBuffer parameter must hold at least ((last-first)/2) elements (i.e. half the elements of the container).
	// This minimum size is a worst-case size requirement, but handles all possible cases. pBuffer is just a scratch
	// buffer and is not needed after the return of this function, and doesn't need to be seeded with any particular
	// values upon entering this function.
	//
	// Example usage:
	//     int intArray[64];
	//     int buffer[32];
	//     ...
	//     tim_sort_buffer(intArray, intArray + 64, buffer);
	//
	template <typename RandomAccessIterator, typename T, typename StrictWeakOrdering>
	void tim_sort_buffer(RandomAccessIterator first, RandomAccessIterator last, T* pBuffer, StrictWeakOrdering compare)
	{
		using namespace Internal;

		// To consider: Convert the implementation to use first/last instead of first/size.
		const intptr_t size = (intptr_t)(last - first);
		if (size == 0)
		{
			// This branch is necessary because the expression `first + 1` below is undefined
			// behaviour when first is nullptr (for example when it is the begin() iterator of an
			// empty vector).
			return;
		}
		else if (size < 64)
		{
			merge_sort_insertion_sort(first, first + 1, first + size, compare);
		}
		else
		{
			tim_sort_run   run_stack[kTimSortStackSize];
			intptr_t       stack_curr = 0;
			intptr_t       len, run;
			intptr_t       curr = 0;
			const intptr_t minrun = timsort_compute_minrun(size);

			#if EASTL_DEV_DEBUG
				memset(run_stack, 0, sizeof(run_stack));
			#endif

			if(tim_sort_add_run<RandomAccessIterator, T, StrictWeakOrdering>(run_stack, first, pBuffer, size, minrun, len, run, curr, stack_curr, compare))
				return;
			if(tim_sort_add_run<RandomAccessIterator, T, StrictWeakOrdering>(run_stack, first, pBuffer, size, minrun, len, run, curr, stack_curr, compare))
				return;
			if(tim_sort_add_run<RandomAccessIterator, T, StrictWeakOrdering>(run_stack, first, pBuffer, size, minrun, len, run, curr, stack_curr, compare))
				return;

			for(;;)
			{
				if(timsort_check_invariant(run_stack, stack_curr))
					stack_curr = tim_sort_collapse<RandomAccessIterator, T, StrictWeakOrdering>(first, run_stack, stack_curr, pBuffer, size, compare);
				else
				{
					if(tim_sort_add_run<RandomAccessIterator, T, StrictWeakOrdering>(run_stack, first, pBuffer, size, minrun, len, run, curr, stack_curr, compare))
						break;
				}
			}
		}
	}


	template <typename RandomAccessIterator, typename T>
	inline void tim_sort_buffer(RandomAccessIterator first, RandomAccessIterator last, T* pBuffer)
	{
		typedef eastl::less<T> Less;

		eastl::tim_sort_buffer<RandomAccessIterator, T, Less>(first, last, pBuffer, Less());
	}




	/// radix_sort
	///
	/// Implements a classic LSD (least significant digit) radix sort.
	/// See http://en.wikipedia.org/wiki/Radix_sort.
	/// This sort requires that the sorted data be of a type that has a member
	/// radix_type typedef and an mKey member of that type. The type must be
	/// an integral type. This limits what can be sorted, but radix_sort is 
	/// very fast -- typically faster than any other sort.
	/// For example:
	///     struct Sortable {
	///         typedef int radix_type;
	///         radix_type mKey;
	///         // User data goes here, or the user can inherit from Sortable.
	///     };
	/// or, more generally:
	///     template <typname Integer>
	///     struct Sortable {
	///         typedef Integer radix_type;
	///         Integer mKey;
	///     };
	/// 
	/// Example usage:
	///     struct Element {
	///         typedef uint16_t radix_type;
	///         uint16_t mKey;
	///         uint16_t mUserData;
	///     };
	///
	///     Element elementArray[100];
	///     Element buffer[100];
	///
	///     radix_sort<Element*, extract_radix_key<Element> >(elementArray, elementArray + 100, buffer);
	///
	/// To consider: A static linked-list implementation may be faster than the version here.

	namespace Internal
	{
		/// extract_radix_key
		///
		/// Default radix sort integer value reader. It expects the sorted elements 
		/// to have an integer member of type radix_type and of name "mKey". 
		///
		template <typename Node>
		struct extract_radix_key
		{
			typedef typename Node::radix_type radix_type;

			const radix_type operator()(const Node& x) const
				{ return x.mKey; }
		};

		// The radix_sort implementation uses two optimizations that are not part of a typical radix sort implementation.
		// 1. Computing a histogram (i.e. finding the number of elements per bucket) for the next pass is done in parallel with the loop that "scatters"
		//    elements in the current pass.  The advantage is that it avoids the memory traffic / cache pressure of reading keys in a separate operation.
		//    Note: It would also be possible to compute all histograms in a single pass.  However, that would increase the amount of stack space used and
		//    also increase cache pressure slightly.  However, it could still be faster under some situations.
		// 2. If all elements are mapped to a single bucket, then there is no need to perform a scatter operation.  Instead the elements are left in place
		//    and only copied if they need to be copied to the final output buffer.
		template <typename RandomAccessIterator, typename ExtractKey, int DigitBits, typename IntegerType>
		void radix_sort_impl(RandomAccessIterator first,
			RandomAccessIterator last,
			RandomAccessIterator buffer,
			ExtractKey extractKey,
			IntegerType)
		{
			if(first == last)
				return;

			RandomAccessIterator srcFirst = first;
			EA_CONSTEXPR_OR_CONST size_t numBuckets = 1 << DigitBits;
			EA_CONSTEXPR_OR_CONST IntegerType bucketMask = numBuckets - 1;

			// The alignment of this variable isn't required; it merely allows the code below to be faster on some platforms.
			uint32_t EA_PREFIX_ALIGN(EASTL_PLATFORM_PREFERRED_ALIGNMENT) bucketSize[numBuckets];
			uint32_t EA_PREFIX_ALIGN(EASTL_PLATFORM_PREFERRED_ALIGNMENT) bucketPosition[numBuckets];

			RandomAccessIterator temp;
			uint32_t i;
			bool doSeparateHistogramCalculation = true;

			constexpr uint32_t kMaxDigitBits = 8 * sizeof(IntegerType);
			for (uint32_t j = 0; j < kMaxDigitBits; j += DigitBits)
			{
				if (doSeparateHistogramCalculation)
				{
					memset(bucketSize, 0, sizeof(bucketSize));
					// Calculate histogram for the first scatter operation
					for (temp = srcFirst; temp != last; ++temp)
						++bucketSize[(extractKey(*temp) >> j) & bucketMask];
				}

				// If a single bucket contains all of the elements, then don't bother redistributing all elements to the
				// same bucket.
				if (bucketSize[((extractKey(*srcFirst) >> j) & bucketMask)] == uint32_t(last - srcFirst))
				{
					// Set flag to ensure histogram is computed for next digit position.
					doSeparateHistogramCalculation = true;
				}
				else
				{
					// The histogram is either not needed or it will be calculated in parallel with the scatter operation below for better cache efficiency.
					doSeparateHistogramCalculation = false;

					// If this is the last digit position, then don't calculate a histogram
					const uint32_t jNext = j + DigitBits;
					if (jNext >= kMaxDigitBits)
					{
						bucketPosition[0] = 0;
						for (i = 0; i < numBuckets - 1; i++)
						{
							bucketPosition[i + 1] = bucketPosition[i] + bucketSize[i];
						}

						for (temp = srcFirst; temp != last; ++temp)
						{
							IntegerType key = extractKey(*temp);
							const size_t digit = (key >> j) & bucketMask;
							buffer[bucketPosition[digit]++] = iter_move(temp);
						}
					}
					// Compute the histogram while performing the scatter operation
					else
					{
						bucketPosition[0] = 0;
						for (i = 0; i < numBuckets - 1; i++)
						{
							bucketPosition[i + 1] = bucketPosition[i] + bucketSize[i];
							bucketSize[i] = 0;	// Clear the bucket for the next pass
						}
						bucketSize[numBuckets - 1] = 0; 

						for (temp = srcFirst; temp != last; ++temp)
						{
							const IntegerType key = extractKey(*temp);
							const size_t digit = (key >> j) & bucketMask;
							buffer[bucketPosition[digit]++] = iter_move(temp);

							// Update histogram for the next scatter operation
							++bucketSize[(key >> jNext) & bucketMask];
						}
					}

					last = buffer + (last - srcFirst);
					temp = srcFirst;
					srcFirst = buffer;
					buffer = temp;
				}
			}

			if (srcFirst != first)
			{
				// Move the values back into the expected buffer
				// GCC 12 to 15 report writes past the end of buffer at -O3; buffer is first here, and the loop
				// moves last - srcFirst elements into it.
				EA_DISABLE_GCC_WARNING(-Wstringop-overflow)
				for (temp = srcFirst; temp != last; ++temp)
					*buffer++ = iter_move(temp);
				EA_RESTORE_GCC_WARNING()
			}
		}
	} // namespace Internal

	template <typename RandomAccessIterator, typename ExtractKey, int DigitBits = 8>
	void radix_sort(RandomAccessIterator first, RandomAccessIterator last, RandomAccessIterator buffer)
	{
		static_assert(DigitBits > 0, "DigitBits must be > 0");
		static_assert(DigitBits <= (sizeof(typename ExtractKey::radix_type) * 8), "DigitBits must be <= the size of the key (in bits)");
		eastl::Internal::radix_sort_impl<RandomAccessIterator, ExtractKey, DigitBits>(first, last, buffer, ExtractKey(), typename ExtractKey::radix_type());
	}



	/// comb_sort
	///
	/// This is an unstable sort.
	/// Implements the CombSort algorithm; in particular, implements the CombSort11 variation 
	/// of the CombSort algorithm, based on the reference to '11' in the implementation.
	///
	/// To consider: Use a comb sort table instead of the '((nSpace * 10) + 3) / 13' expression.
	///              Ideal tables can be found on the Internet by looking up "comb sort table".
	///
	template <typename ForwardIterator, typename StrictWeakOrdering>
	void comb_sort(ForwardIterator first, ForwardIterator last, StrictWeakOrdering compare)
	{
		typedef typename eastl::iterator_traits<ForwardIterator>::difference_type difference_type;

		ForwardIterator iCurrent, iNext;
		difference_type length = eastl::distance(first, last);
		difference_type nSpace = length;

		for(bool bSwapped = false; (nSpace > 1) || bSwapped; )
		{
			nSpace = ((nSpace * 10) + 3) / 13; // Integer division is less than ideal.

			if((nSpace == 9) || (nSpace == 10))
				nSpace = 11;

			iCurrent = iNext = first;
			eastl::advance(iNext, nSpace);
			bSwapped = false;

			if(nSpace > 1)
			{
				for(; iNext != last; iCurrent++, (void)iNext++)
				{
					if(compare(*iNext, *iCurrent))
					{
						EASTL_VALIDATE_COMPARE(!compare(*iCurrent, *iNext)); // Validate that the compare function is sane.
						eastl::iter_swap(iCurrent, iNext);
						bSwapped = true;
					}
				}
			}
			else
			{
				// With a gap of 1 this is a bubble sort pass: the elements from the last one swapped on are in their final
				// place, so the next pass ends there. It also makes each pass shorter than the previous one, whatever the
				// comparator answers, which a comparator that always reports a pair out of order would otherwise never end.
				// (Only this pass tracks the last swap: tracking it in every pass made comb_sort up to 7% slower on pairs.)
				ForwardIterator iLastSwapped = last;

				for(; iNext != last; iCurrent++, (void)iNext++)
				{
					if(compare(*iNext, *iCurrent))
					{
						EASTL_VALIDATE_COMPARE(!compare(*iCurrent, *iNext)); // Validate that the compare function is sane.
						eastl::iter_swap(iCurrent, iNext);
						bSwapped = true;
						iLastSwapped = iNext;
					}
				}

				last = iLastSwapped;
			}
		}
	} // comb_sort

	template <typename ForwardIterator>
	inline void comb_sort(ForwardIterator first, ForwardIterator last)
	{
		typedef eastl::less<typename eastl::iterator_traits<ForwardIterator>::value_type> Less;

		eastl::comb_sort<ForwardIterator, Less>(first, last, Less());
	}




	/// bubble_sort
	///
	/// This is a stable sort.
	/// Implements the BubbleSort algorithm. This algorithm is only useful for 
	/// small range sizes, such as 10 or less items. You may be better off using
	/// insertion_sort for cases where bubble_sort works.
	///
	namespace Internal
	{
		template <typename ForwardIterator, typename StrictWeakOrdering>
		void bubble_sort_impl(ForwardIterator first, ForwardIterator last, StrictWeakOrdering compare, eastl::forward_iterator_tag)
		{
			ForwardIterator iCurrent, iNext;

			while(first != last)
			{
				iNext = iCurrent = first;
				
				for(++iNext; iNext != last; iCurrent = iNext, (void)++iNext) 
				{
					if(compare(*iNext, *iCurrent))
					{
						EASTL_VALIDATE_COMPARE(!compare(*iCurrent, *iNext)); // Validate that the compare function is sane.
						eastl::iter_swap(iCurrent, iNext);
					}
				}
				last = iCurrent;
			}
		}

		template <typename BidirectionalIterator, typename StrictWeakOrdering>
		void bubble_sort_impl(BidirectionalIterator first, BidirectionalIterator last, StrictWeakOrdering compare, eastl::bidirectional_iterator_tag)
		{
			if(first != last)
			{
				BidirectionalIterator iCurrent, iNext, iLastModified;

				last--;

				while(first != last)
				{
					iLastModified = iNext = iCurrent = first;
					
					for(++iNext; iCurrent != last; iCurrent = iNext, (void)++iNext)
					{
						if(compare(*iNext, *iCurrent))
						{
							EASTL_VALIDATE_COMPARE(!compare(*iCurrent, *iNext)); // Validate that the compare function is sane.
							iLastModified = iCurrent;
							eastl::iter_swap(iCurrent, iNext);
						}
					}

					last = iLastModified;
				}
			}
		}
	} // namespace Internal

	template <typename ForwardIterator, typename StrictWeakOrdering>
	inline void bubble_sort(ForwardIterator first, ForwardIterator last, StrictWeakOrdering compare)
	{
		typedef typename eastl::iterator_traits<ForwardIterator>::iterator_category IC;

		eastl::Internal::bubble_sort_impl<ForwardIterator, StrictWeakOrdering>(first, last, compare, IC());
	}

	template <typename ForwardIterator>
	inline void bubble_sort(ForwardIterator first, ForwardIterator last)
	{
		typedef eastl::less<typename eastl::iterator_traits<ForwardIterator>::value_type> Less;
		typedef typename eastl::iterator_traits<ForwardIterator>::iterator_category IC;

		eastl::Internal::bubble_sort_impl<ForwardIterator, Less>(first, last, Less(), IC());
	}



	/// sort
	/// 
	/// We use quick_sort by default. See quick_sort for details.
	///
	/// EASTL_DEFAULT_SORT_FUNCTION
	/// If a default sort function is specified then call it, otherwise use EASTL's default quick_sort.
	/// EASTL_DEFAULT_SORT_FUNCTION must be namespace-qualified and include any necessary template
	/// parameters (e.g. eastl::comb_sort instead of just comb_sort), and it must be visible to this code. 
	/// The EASTL_DEFAULT_SORT_FUNCTION must be provided in two versions: 
	///     template <typename RandomAccessIterator>
	///     void EASTL_DEFAULT_SORT_FUNCTION(RandomAccessIterator first, RandomAccessIterator last);
	///
	///     template <typename RandomAccessIterator, typename Compare>
	///     void EASTL_DEFAULT_SORT_FUNCTION(RandomAccessIterator first, RandomAccessIterator last, Compare compare)
	///
	template <typename RandomAccessIterator>
	inline void sort(RandomAccessIterator first, RandomAccessIterator last)
	{
		#if defined(EASTL_DEFAULT_SORT_FUNCTION)
			EASTL_DEFAULT_SORT_FUNCTION(first, last);
		#else
			eastl::quick_sort<RandomAccessIterator>(first, last);
		#endif
	}

	template <typename RandomAccessIterator, typename Compare>
	inline void sort(RandomAccessIterator first, RandomAccessIterator last, Compare compare)
	{
		#if defined(EASTL_DEFAULT_SORT_FUNCTION)
			EASTL_DEFAULT_SORT_FUNCTION(first, last, compare);
		#else
			eastl::quick_sort<RandomAccessIterator, Compare>(first, last, compare);
		#endif
	}



	/// stable_sort
	/// 
	/// We use merge_sort by default. See merge_sort for details.
	/// Beware that the used merge_sort -- and thus stable_sort -- allocates 
	/// memory during execution. Try using merge_sort_buffer if you want
	/// to avoid memory allocation.
	///
	/// EASTL_DEFAULT_STABLE_SORT_FUNCTION
	/// If a default sort function is specified then call it, otherwise use EASTL's default merge_sort.
	/// EASTL_DEFAULT_STABLE_SORT_FUNCTION must be namespace-qualified and include any necessary template
	/// parameters (e.g. eastl::tim_sort instead of just tim_sort), and it must be visible to this code. 
	/// The EASTL_DEFAULT_STABLE_SORT_FUNCTION must be provided in three versions, though the third
	/// allocation implementation may choose to ignore the allocator parameter: 
	///     template <typename RandomAccessIterator, typename StrictWeakOrdering>
	///     void EASTL_DEFAULT_STABLE_SORT_FUNCTION(RandomAccessIterator first, RandomAccessIterator last, StrictWeakOrdering compare);
	///     
	///     template <typename RandomAccessIterator>
	///     void EASTL_DEFAULT_STABLE_SORT_FUNCTION(RandomAccessIterator first, RandomAccessIterator last);
	///
	///     template <typename RandomAccessIterator, typename Allocator, typename StrictWeakOrdering>
	///     void EASTL_DEFAULT_STABLE_SORT_FUNCTION(RandomAccessIterator first, RandomAccessIterator last, Allocator& allocator, StrictWeakOrdering compare);
	///
	template <typename RandomAccessIterator, typename StrictWeakOrdering>
	void stable_sort(RandomAccessIterator first, RandomAccessIterator last, StrictWeakOrdering compare)
	{
		#if defined(EASTL_DEFAULT_STABLE_SORT_FUNCTION)
			EASTL_DEFAULT_STABLE_SORT_FUNCTION(first, last, *get_default_allocator(0), compare);
		#else
			eastl::merge_sort<RandomAccessIterator, EASTLAllocatorType, StrictWeakOrdering>
							 (first, last, *get_default_allocator(0), compare);
		#endif
	}

	template <typename RandomAccessIterator>
	void stable_sort(RandomAccessIterator first, RandomAccessIterator last)
	{
		#if defined(EASTL_DEFAULT_STABLE_SORT_FUNCTION)
			EASTL_DEFAULT_STABLE_SORT_FUNCTION(first, last, *get_default_allocator(0));
		#else
			eastl::merge_sort<RandomAccessIterator, EASTLAllocatorType>
							 (first, last, *get_default_allocator(0));
		#endif
	}

	template <typename RandomAccessIterator, typename Allocator, typename StrictWeakOrdering>
	void stable_sort(RandomAccessIterator first, RandomAccessIterator last, Allocator& allocator, StrictWeakOrdering compare)
	{
		#if defined(EASTL_DEFAULT_STABLE_SORT_FUNCTION)
			EASTL_DEFAULT_STABLE_SORT_FUNCTION(first, last, allocator, compare);
		#else
			eastl::merge_sort<RandomAccessIterator, Allocator, StrictWeakOrdering>(first, last, allocator, compare);
		#endif
	}

	// This is not defined because it would cause compiler errors due to conflicts with a version above. 
	//template <typename RandomAccessIterator, typename Allocator>
	//void stable_sort(RandomAccessIterator first, RandomAccessIterator last, Allocator& allocator)
	//{
	//    #if defined(EASTL_DEFAULT_STABLE_SORT_FUNCTION)
	//        EASTL_DEFAULT_STABLE_SORT_FUNCTION<RandomAccessIterator, Allocator>(first, last, allocator);
	//    #else
	//        eastl::merge_sort<RandomAccessIterator, Allocator>(first, last, allocator);
	//    #endif
	//}




	/* 
	// Something to consider adding: An eastl sort which uses qsort underneath. 
	// The primary purpose of this is to have an eastl interface for sorting which
	// results in very little code generation, since all instances map to the 
	// C qsort function.

	template <typename T>
	int small_footprint_sort_func(const void* a, const void* b)
	{
		if(*(const T*)a < *(const T*)b)
			return -1;
		if(*(const T*)a > *(const T*)b)
			return +1;
		return 0;
	}

	template <typename ContiguousIterator>
	void small_footprint_sort(ContiguousIterator first, ContiguousIterator last)
	{
		typedef typename eastl::iterator_traits<ContiguousIterator>::value_type value_type;

		qsort(first, (size_t)eastl::distance(first, last), sizeof(value_type), small_footprint_sort_func<value_type>);
	}
	*/

} // namespace eastl


#endif // Header include guard



