/***********************************************************************[sort.hpp]
Copyright(c) 2022, Muhammad Osama - Anton Wijs,
Technische Universiteit Eindhoven (TU/e).

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
**********************************************************************************/

#ifndef __SORT_
#define __SORT_

#include "key.hpp"
#include "pdqsort.hpp"
#include "quadsort.hpp"
#include "wolfsort.hpp"
#include "radixsort.hpp"

#undef SORT
#undef QSORTCMP
#undef STABLESORT
#undef INSORT_THR

namespace SeqFROST {

	#define INSORT_THR	24

#if defined(STDSORTSTB)
	#define STABLESORT(PTR, END, LEN, CMP) std::stable_sort((PTR), (END), (CMP))
#else 
	#define STABLESORT(PTR, END, LEN, CMP) quadsort((PTR), (LEN), (CMP))
#endif


	#define SORT(SRC) \
	do { \
		if (SRC.empty()) break; \
		if (SRC.size() <= INSORT_THR)	\
			insertionSort(SRC.data(), SRC.size()); \
		else \
			pdqsort_branchless(SRC.data(), SRC.end()); \
		assert(isSorted(SRC.data(), SRC.size())); \
	} while (0)

#	define QSORTCMP(SRC, CMP) \
	do { \
		if (SRC.empty()) break; \
		if (SRC.size() <= INSORT_THR) \
			insertionSort(SRC.data(), SRC.size(), CMP); \
		else \
			pdqsort(SRC.data(), SRC.end(), CMP); \
		assert(isSorted(SRC.data(), SRC.size(), CMP)); \
	} while(0)

	#define SORTCMP(SRC, CMP) \
	do { \
		if (SRC.empty()) break; \
		if (SRC.size() <= INSORT_THR) \
			insertionSort(SRC.data(), SRC.size(), CMP); \
		else \
			std::sort(SRC.data(), SRC.end(), CMP); \
		assert(isSorted(SRC.data(), SRC.size(), CMP)); \
	} while (0)

	//============================//
	//  Sorting Helpers           //
	//============================//
	template<class T, class SZ, class CMP = LESS<T>()>
	inline bool isSorted(T* d, const SZ& size, CMP cmp) 
	{
		for (SZ i = 1; i < size; ++i)
			if (cmp(d[i], d[i - 1])) 
				return false;
		return true;
	}

	template<class T, class SZ>
	inline bool isSorted(T* d, const SZ& size) 
	{
		for (SZ i = 1; i < size; ++i)
			if (d[i] < d[i - 1]) 
				return false;
		return true;
	}

	template<class T, class SZ, class CMP>
	inline void insertionSort(T* d, const SZ& size, CMP cmp)
	{
		if (size == 2 && cmp(d[1], d[0]))
			std::swap(d[1], d[0]);
		else if (size > 2) {
			T* end = d + size;
			for (T* i = d + 1; i != end; ++i) {
				T*  j = i, *j_1 = i - 1;
				if (cmp(*j, *j_1)) {
					T tmp = *j;
					do { *j-- = *j_1; } 
					while (j != d && cmp(tmp, *--j_1));
					*j = tmp;
				}
			}
		}
	}

	template<class T, class SZ>
	inline void insertionSort(T* d, const SZ& size)
	{
		if (size == 2 && d[1] < d[0])
			std::swap(d[1], d[0]);
		else if (size > 2) {
			T* end = d + size;
			for (T* i = d + 1; i != end; ++i) {
				T*  j = i, *j_1 = i - 1;
				if (*j < *j_1) {
					T tmp = *j;
					do { *j-- = *j_1; } 
					while (j != d && (tmp < *--j_1));
					*j = tmp;
				}
			}
		}
	}

}

#endif