/***********************************************************************[queue.hpp]
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

#ifndef __QUEUE_
#define __QUEUE_

#include "malloc.hpp"
#include "definitions.hpp"

namespace SeqFROST {

	struct LINK {
		uint32 prev, next;
		LINK() : prev(0), next(0) {}
	};

	class QUEUE {

		LINK* links;
		uint32 sz, cap;
		uint64 _bumped;
		uint32 _first, _last, _free;

		__forceinline void		inQue		(const uint32& v) {
			CHECKVAR(v);

			LINK& link = links[v];

			link.prev = _last;

			if (_last) 
				(links + _last)->next = v;
			else 
				_first = v;

			_last = v;

			link.next = 0;
		}
		__forceinline void		outQue		(const uint32& v) {
			CHECKVAR(v);

			LINK& link = links[v];

			if (link.prev)
				(links + link.prev)->next = link.next;
			else
				_first = link.next;

			if (link.next) 
				(links + link.next)->prev = link.prev;
			else
				_last = link.prev;
		}

	public:

		QUEUE() : 
			_bumped(0)
			, _first(0)
			, _last(0)
			, _free(0) 
		{ }

		~QUEUE() { 
			if (links)
				std::free(links), links = NULL, sz = cap = 0;
		}

		__forceinline void		reserve		(const uint32& size) { 
			assert(size);
			RESERVE(LINK, UINT32_MAX, links, cap, size + 1); 
		}
		__forceinline void		insert		(const uint32& v) 
		{
			CHECKVAR(v);

			const uint32 newsize = v + 1;
			if (sz < newsize) {
				RESERVE(LINK, UINT32_MAX, links, cap, newsize);
				sz = newsize;
			}

			LINK& link = links[v];

			link.next = 0;

			if (_last) {
				assert(!links[_last].next);
				(links + _last)->next = v;
			}
			else {
				assert(!_first);
				_first = v;
			}

			link.prev = _last;

			_last = v;
		}
		__forceinline void		map			(uint32* mapped, const uint32& firstDL0, const uint32& newsize) 
		{
			// map nodes
			uint32 q, _next, _prev = 0, _mPrev = 0;

			for (q = _first; q; q = _next) {

				_next = (links + q)->next;

				if (q == firstDL0) continue;

				uint32 mVar = mapped[q];
				if (mVar) {

					if (_prev)
						(links + _prev)->next = mVar;
					else 
						_first = mVar;

					(links + q)->prev = _mPrev;

					_mPrev = mVar;
					_prev = q;
				}
			}

			if (_prev) 
				(links + _prev)->next = 0;
			else 
				_first = 0;

			_free = _last = _mPrev;

			// map links
			assert(inf.maxVar >= newsize);

			forall_variables(v) {
				uint32 mVar = mapped[v];
				if (mVar) links[mVar] = links[v];
			}

			assert(sz >= newsize);

			sz = newsize;

			if (cap > sz) {
				sfshrinkAlloc(links, sizeof(LINK) * sz);
				cap = sz;
			}
		}
		__forceinline void		update		(const uint32& v, const uint64& bump) { 
			CHECKVAR(v);

			_free = v, _bumped = bump;

		#ifdef LOGGING
			LOG2(4, "  queue free updated to (v: %d, bump: %lld)", _free, _bumped); 
		#endif
		}
		__forceinline void		toFront		(const uint32& v) { CHECKVAR(v); outQue(v), inQue(v); }
		__forceinline uint32	previous	(const uint32& v) { CHECKVAR(v); return (links + v)->prev; }
		__forceinline uint32	next		(const uint32& v) { CHECKVAR(v); return (links + v)->next; }
		__forceinline LINK*		data		() { return links; }
		__forceinline uint32	free		() { return _free; }
		__forceinline uint32	first		() { return _first; }
		__forceinline uint32	last		() { return _last; }
		__forceinline uint64	bumped		() { return _bumped; }
					  void		print		() {
			LOG1(" Queue (first: %d, last: %d, free: %d, bumped: %lld):", _first, _last, _free, _bumped);
			for (uint32 i = 0; i < sz; ++i) 
				LOG1(" Q(%d)->(p: %d, n: %d)", i, links[i].prev, links[i].next);
		}
	};

}

#endif 