/***********************************************************************[heap.hpp]
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

#ifndef __HEAP_
#define __HEAP_

#include "vector.hpp"
#include "definitions.hpp"

namespace SeqFROST {

	constexpr uint32 ILLEGAL_POS = 0xFFFFFFFF;

	template <class CMP>
	class HEAP {

		uint32*	heap, heapsize, heapcap;
		uint32*	pos,  possize, poscap;
		CMP		cmp;

		__forceinline uint32	left		(const uint32& i) { return (i << 1) + 1; }
		__forceinline uint32	parent		(const uint32& i) { return (i - 1) >> 1; }
		__forceinline void		bubbleUp	(const uint32& x) {
			assert(x < ILLEGAL_POS);
			uint32 xpos = pos[x];
			while (xpos) {
				uint32 ppos = parent(xpos);
				uint32 p = heap[ppos];
				if (cmp(p, x)) {
					heap[xpos] = p;
					pos[p] = xpos;
					xpos = ppos;
				}
				else
					break;
			}
			heap[xpos] = x;
			pos[x] = xpos;
		}
		__forceinline void		bubbleDown	(const uint32& x) {
			assert(x < ILLEGAL_POS);
			uint32 xpos = pos[x], childpos = left(xpos);
			while (childpos < heapsize) {
				uint32 child = heap[childpos];
				const uint32 otherpos = childpos + 1;
				if (otherpos < heapsize) {
					const uint32 other = heap[otherpos];
					if (cmp(child, other)) {
						child = other;
						childpos = otherpos;
					}
				}
				if (cmp(x, child)) {
					heap[xpos] = child;
					pos[child] = xpos;
					xpos = childpos;
					childpos = left(xpos);
				}
				else 
					break;
			}
			heap[xpos] = x;
			pos[x] = xpos;
		}
	public:

		__forceinline uint32*	data		() { return heap; }
		__forceinline uint32*	end			() { return heap + heapsize; }
		__forceinline uint32	top			() { assert(heapsize); return *heap; }
		__forceinline uint32	size		() const { return heapsize; }
		__forceinline bool		empty		() const { return !heapsize; }
		__forceinline uint32	pop			() {
			uint32 x = *heap;
			assert(heapsize >= 1);
			uint32 last = heap[--heapsize];
			pos[last] = ILLEGAL_POS;
			if (last == x)
				return x;
			pos[x] = ILLEGAL_POS;
			*heap = last;
			pos[last] = 0;
			bubbleDown(last);
			return x;
		}
		__forceinline uint32&	operator[]	(const uint32& i) { assert(i < heapsize); return heap[i]; }
		__forceinline bool		has			(const uint32& x) const { return x < possize && NEQUAL(pos[x], ILLEGAL_POS); }
		__forceinline void		update		(const uint32& x) {
			if (has(x)) {
				bubbleUp(x);
			}
		}
		__forceinline void		update		(const uint32& x, const double& oldscore, const double& newscore) {
			if (has(x)) {
				if (newscore > oldscore)
					bubbleUp(x);
				else
					bubbleDown(x);
			}
		}
		__forceinline void		insert		(const uint32& x) {
			assert(!has(x));
			expand(x + 1, ILLEGAL_POS);
			pos[x] = heapsize;
			push(x);
			bubbleUp(x);
		}
		__forceinline void		rebuild		(uVec1D& vars) {
			destroy();
			reserve(vars.size());
			forall_vector(uint32, vars, i)
				insert(*i);
		}
		__forceinline void		push		(const uint32& val) { 
			if (heapsize == heapcap) 
				RESERVE(uint32, ILLEGAL_POS, heap, heapcap, heapsize + 1); 
			heap[heapsize++] = val;
		}
		__forceinline void		expand		(const uint32& size, const uint32& val) {
			if (possize >= size) return;
			RESERVE(uint32, ILLEGAL_POS, pos, poscap, size);
			for (uint32 i = possize; i < size; ++i) 
				pos[i] = val;
			possize = size;
		}
		__forceinline void		reserve		(const uint32& size) { 
			assert(size);
			RESERVE(uint32, ILLEGAL_POS, pos, poscap, size + 1);
			RESERVE(uint32, ILLEGAL_POS, heap, heapcap, size); 
		}
		__forceinline void		clear		() {
			const uint32* e = heap + heapsize;
			for (uint32* i = heap; i != e; ++i)
				pos[*i] = ILLEGAL_POS;
			heapsize = 0;
		}
		__forceinline void		destroy		() {
			if (heap)
				std::free(heap), heap = NULL, heapsize = 0, heapcap = 0;
			if (pos)
				std::free(pos), pos = NULL, possize = 0, poscap = 0;
		}

		HEAP(const CMP& _cmp) :
			heap(NULL)
			, heapsize(0)
			, heapcap(0)
			, pos(NULL)
			, possize(0)
			, poscap(0)
			, cmp(_cmp)
		{ }

		HEAP() :
			heap(NULL)
			, heapsize(0)
			, heapcap(0)
			, pos(NULL)
			, possize(0)
			, poscap(0)
		{ }

		~HEAP() { destroy(); }
		
	};

}

#endif