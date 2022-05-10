/***********************************************************************[vector.hpp]
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

#ifndef __VECTOR_
#define __VECTOR_

#include "malloc.hpp"
#include "color.hpp"
#include <cstdlib>
#include <limits>
#include <cassert>
#include <typeinfo>
#include <iostream>

#ifdef __GNUC__
#define __forceinline __attribute__((always_inline))
#endif

namespace SeqFROST {

	template<class T, class S = uint32>
	class Vec {

		T* _mem;
		S sz, cap;
		const S maxCap;

		#define CONSTRUCT(NEWSIZE) while (sz < (NEWSIZE)) { new (_mem + sz) T(); ++sz; }

		#define DECONSTRUCT(NEWSIZE) while (sz > (NEWSIZE)) { --sz; (_mem + sz)->~T(); }

		#define VECTOR_RESERVE(MINCAP) RESERVE(T, maxCap, _mem, cap, MINCAP)

		#define VECTOR_INIT(OFF,N,VAL) \
		{ \
			assert((OFF) <= (N)); \
			assert((N) <= cap); \
			T* begin = _mem + (OFF); \
			if (!VAL) \
				std::memset(begin, 0, ((N) - (OFF)) * sizeof(T)); \
			else { \
				const T* end = _mem + (N); \
				while (begin != end) *begin++ = VAL; \
			} \
		}

		__forceinline bool		check		(const S& idx) const {
			if (idx >= sz) {
				SETCOLOR(CERROR, stderr);
				std::cerr << "ERROR - index is out of vector boundary (type: " << typeid(T).name() <<
					", index: " << (long long)idx << ", size:" << (long long)sz << ")" << std::endl;
				SETCOLOR(CNORMAL, stderr);
				return false;
			}
			return true;
		}

	public:

		__forceinline			~Vec		() { clear(true); }

		__forceinline			Vec			() :
			_mem(NULL)
			, sz(0)
			, cap(0)
			, maxCap(std::numeric_limits<S>::max())
		{ }

		__forceinline explicit	Vec			(const S& size) :
			_mem(NULL)
			, sz(0)
			, cap(0)
			, maxCap(std::numeric_limits<S>::max())
		{
			resize(size);
		}

		__forceinline			Vec			(const S& size, const T& val) :
			_mem(NULL)
			, sz(0)
			, cap(0)
			, maxCap(std::numeric_limits<S>::max()) 
		{
			resize(size, val);
		}

		__forceinline Vec<T>&	operator=	(Vec<T>& rhs) { return *this; }
		__forceinline const T&	operator[]	(const S& index) const { assert(check(index)); return _mem[index]; }
		__forceinline T&		operator[]	(const S& index) { assert(check(index)); return _mem[index]; }
		__forceinline const T&	back		() const { assert(sz); return _mem[sz - 1]; }
		__forceinline T&		back		() { assert(sz); return _mem[sz - 1]; }
		__forceinline			operator T* () { return _mem; }
		__forceinline T*		data		() { return _mem; }
		__forceinline T*		end			() { return _mem + sz; }
		__forceinline bool		empty		() const { return !sz; }
		__forceinline S			size		() const { return sz; }
		__forceinline S			capacity	() const { return cap; }
		__forceinline void		pop			() { 
			assert(sz > 0); 
			--sz;
			(_mem + sz)->~T(); 
		}
		__forceinline void		insert		(const T& val) { assert(cap > sz);  _mem[sz++] = val; }
		__forceinline void		push		(const T& val) {
			if (sz == cap) {
				VECTOR_RESERVE(sz + 1);
			}
			new (_mem + sz) T(val); 
			++sz; 
		}
		__forceinline void		resize		(const S& newsize) {
			if (newsize > sz) {
				VECTOR_RESERVE(newsize);
				CONSTRUCT(newsize);
			}
			else if (newsize < sz) {
				DECONSTRUCT(newsize);
			}
		}
		__forceinline void		resize		(const S& newsize, const T& val) {
			if (newsize > sz) {
				VECTOR_RESERVE(newsize);
				sz = newsize;
			}
			else if (newsize < sz) {
				DECONSTRUCT(newsize);
			}
			VECTOR_INIT(0, newsize, val); 

		}
		__forceinline void		shrink		(const S& shrunken) {
			assert(shrunken <= sz);
			const S newsize = sz - shrunken;
			DECONSTRUCT(newsize);
		}
		__forceinline void		expand		(const S& newsize) {
			if (sz < newsize) {
				VECTOR_RESERVE(newsize);
				CONSTRUCT(newsize);
			}
		}
		__forceinline void		expand		(const S& newsize, const T& val) {
			if (sz < newsize) {
				VECTOR_RESERVE(newsize);
				VECTOR_INIT(sz, newsize, val);
				sz = newsize;
			}
		}
		__forceinline void		reserve		(const S& mincap) {
			VECTOR_RESERVE(mincap);
		}
		__forceinline void		shrinkCap	() {
			if (!sz) 
				clear(true);
			else if (cap > sz) {
				sfshrinkAlloc(_mem, sizeof(T) * sz);
				cap = sz;
			}
		}
		__forceinline void		copyFrom	(Vec<T, S>& src) {
			resize(src.size());
			std::memcpy(_mem, src, sz * sizeof(T));
		}
		__forceinline void		moveFrom	(Vec<T, S>& src) {
			copyFrom(src);
			src.clear(true);
		}
		__forceinline void		migrateTo	(Vec<T, S>& dest) {
			if (dest._mem) 
				std::free(dest._mem);
			dest._mem = _mem, dest.sz = sz, dest.cap = cap;
			_mem = NULL, sz = cap = 0;
		}
		__forceinline void		clear		(const bool& _free = false) {
			if (_mem) {
				const T* e = _mem + sz;
				for (T* i = _mem; i != e; ++i) i->~T();
				sz = 0;
				if (_free) { 
					std::free(_mem);
					_mem = NULL;
					cap = 0;
				}
			}
		}
	};

	// vector types
	typedef Vec<int> Vec1D;
	typedef Vec<uint32> uVec1D;
	typedef Vec<uint32, int> Lits_t;
	typedef Vec<double> score_t;
	typedef Vec<uint64> bump_t;
}

#endif
