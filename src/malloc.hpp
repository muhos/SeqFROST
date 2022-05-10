/***********************************************************************[malloc.hpp]
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

#ifndef __RALLOC_
#define __RALLOC_

#include "logging.hpp"
#include <cstdlib>
#include <cstring>

namespace SeqFROST {

	class MEMOUTEXCEPTION {};

	template <class T>
	T* sfmalloc(const size_t& numElements) {
		if (!numElements) LOGERR("catched zero-memory size at %s", __func__);
		T* _mem = (T*)std::malloc(numElements * sizeof(T));
		if (!_mem) throw MEMOUTEXCEPTION();
		return _mem;
	}

	template <class T>
	T* sfcalloc(const size_t& numElements) {
		if (!numElements) LOGERR("catched zero-memory size at %s", __func__);
		T* _mem = (T*)std::calloc(numElements, sizeof(T));
		if (!_mem) throw MEMOUTEXCEPTION();
		return _mem;
	}

#if defined(__linux__) || defined(__CYGWIN__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif

	template <class T>
	void sfralloc(T*& mem, const size_t& bytes) {
		if (!bytes) LOGERR("catched zero-memory size at %s", __func__);
		T* _mem = (T*)std::realloc(mem, bytes);
		if (!_mem) throw MEMOUTEXCEPTION();
		mem = _mem;
	}

	template <class T>
	void sfshrinkAlloc(T*& mem, const size_t& bytes) {
		if (!bytes) LOGERR("catched zero-memory size at %s", __func__);
		T* _mem = NULL;
		_mem = (T*)std::realloc(_mem, bytes);
		if (!_mem) throw MEMOUTEXCEPTION();
		std::memcpy(_mem, mem, bytes);
		std::free(mem);
		mem = _mem;
	}

	#define RESERVE(DATATYPE,MAXCAP,MEM,CAP,MINCAP) \
		if (CAP < (MINCAP)) {	\
			CAP = (CAP > ((MAXCAP) - CAP)) ? (MINCAP) : (CAP << 1);	\
			if (CAP < (MINCAP)) CAP = (MINCAP);	\
			sfralloc(MEM, sizeof(DATATYPE) * CAP);	\
		}

#if defined(__linux__) || defined(__CYGWIN__)
#pragma GCC diagnostic pop
#endif

}

#endif