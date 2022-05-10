/***********************************************************************[watch.hpp]
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

#ifndef __WATCH_
#define __WATCH_

#include "clause.hpp"
#include "vector.hpp"
#include "definitions.hpp"

namespace SeqFROST {

	struct WATCH {
		C_REF	ref;
		uint32	imp;
		int		size;

		inline		WATCH	() : 
			ref(UNDEF_REF)
			, imp(0)
			, size(0) 
		{ }

		inline		WATCH	(const C_REF& ref, const int& size, const uint32& imp) :
			ref(ref)
			, imp(imp)
			, size(size) 
		{ }

		inline bool binary	() const { return size == 2; }
	};

	struct DWATCH {
		C_REF	ref;
		uint32	lit, imp;
		int		size;

		inline		DWATCH	() : 
			ref(UNDEF_REF)
			, lit(0)
			, imp(0)
			, size(0) 
		{ }

		inline		DWATCH	(const uint32& lit, const uint32& imp, const C_REF& ref, const int& size) :
			ref(ref)
			, lit(lit)
			, imp(imp)
			, size(size) 
		{ }
	};


	#define forall_watches(WS, PTR) \
		for (WATCH* PTR = WS, *WSEND = WS.end(); PTR != WSEND; ++PTR)

	#define forall_dwatches(DWS, PTR) \
		for (DWATCH* PTR = DWS, *DWSEND = DWS.end(); PTR != DWSEND; ++PTR)

	#define PRIORALLBINS(CODE) (CODE & 1)

	#define PRIORLEARNTBINS(CODE) (CODE & 2)

	#define ATTACH_WATCH(LIT,IMP,REF,SIZE)			\
	{												\
		CHECKLIT(LIT);								\
		CHECKLIT(IMP);								\
		assert(SIZE > 1);							\
		wt[FLIP(LIT)].push(WATCH(REF, SIZE, IMP));	\
	}												\

	#define ATTACH_TWO_WATCHES(REF,C)					\
	{													\
		const int size = C.size();						\
		assert(size > 1);								\
		const uint32 first = C[0], second = C[1];		\
		CHECKLIT(first);								\
		CHECKLIT(second);								\
		wt[FLIP(first)].push(WATCH(REF, size, second)); \
		wt[FLIP(second)].push(WATCH(REF, size, first)); \
	}													\

	#define DELAY_WATCH(LIT,IMP,REF,SIZE)			\
	{												\
		CHECKLIT(LIT);								\
		CHECKLIT(IMP);								\
		assert(SIZE > 1);							\
		dwatches.push(DWATCH(LIT, IMP, REF, SIZE));	\
	}												\

	#define REATTACH_DELAYED									   \
	{															   \
		forall_dwatches(dwatches, d) {							   \
			wt[FLIP(d->lit)].push(WATCH(d->ref, d->size, d->imp)); \
		}														   \
		dwatches.clear();										   \
	}															   \

	#define ATTACH_BINARY(REF,X,Y)					\
	{												\
		wot[X].push(REF);							\
		wot[Y].push(REF);							\
	}												\

	#define ATTACH_TERNARY(REF,X,Y,Z)				\
	{												\
		wot[X].push(REF);							\
		wot[Y].push(REF);							\
		wot[Z].push(REF);							\
	}												\

	#define ATTACH_CLAUSE(REF,C)					\
	{												\
		assert(C.size() > 1);						\
		forall_clause(C, i) {						\
			wot[*i].push(REF);						\
		}											\
	}												\

}

#endif