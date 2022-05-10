/***********************************************************************[prophelper.hpp]
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

#ifndef __PROPHELPER_
#define __PROPHELPER_

namespace SeqFROST {

	#define ADVANCE_WATCHES(W,IMP,IMPVAL,REF,PREPTR,POSTPTR,VALUES)		\
		const WATCH W = *POSTPTR++ = *PREPTR++;							\
		const uint32 IMP = W.imp;										\
		CHECKLIT(IMP);													\
		const LIT_ST IMPVAL = VALUES[IMP];								\
		if (IMPVAL > 0) continue;										\
		const C_REF REF = W.ref;										\


	#define PREFETCH_LARGE_CLAUSE(C,LITS,OTHER,OTHERVAL,REF,FLIPPED,	\
												VALUES, BASEADDRESS)	\
		CLAUSE& C = (CLAUSE&)BASEADDRESS[REF];							\
		assert(C.size() > 2);											\
		assert(C[0] != C[1]);											\
		uint32* LITS = c.data();										\
		const uint32 OTHER = *LITS ^ *(LITS + 1) ^ FLIPPED;				\
		CHECKLIT(OTHER);												\
		const LIT_ST OTHERVAL = VALUES[OTHER];							\


	#define GET_LARGE_CLAUSE(C,LITS,OTHER,OTHERVAL,REF,FLIPPED,VALUES)	\
		CLAUSE& C = cm[REF];											\
		assert(C.size() > 2);											\
		assert(C[0] != C[1]);											\
		uint32* LITS = c.data();										\
		const uint32 OTHER = *LITS ^ *(LITS + 1) ^ FLIPPED;				\
		CHECKLIT(OTHER);												\
		const LIT_ST OTHERVAL = VALUES[OTHER];							\


	// search for (un)-assigned-1 literal to watch
	#define FIND_NEW_WATCH(C,LITS,CPOS,SIZE,CPTR,NEWLIT,VALUES)			\
		uint32* MID = LITS + CPOS;										\
		uint32* END = LITS + SIZE;										\
		uint32* CPTR = MID;												\
		uint32 NEWLIT = 0;												\
		LIT_ST _FALSE_ = UNDEF_VAL;										\
		while (CPTR != END && (_FALSE_ = !VALUES[NEWLIT = *CPTR]))		\
			CPTR++;														\
		assert(_FALSE_ != UNDEF_VAL);									\
		if (_FALSE_) {													\
			CPTR = LITS + 2;											\
			assert(CPOS <= SIZE);										\
			while (CPTR != MID && (_FALSE_ = !VALUES[NEWLIT = *CPTR]))	\
				CPTR++;													\
		}																\
		assert(CPTR >= LITS + 2 && CPTR <= END);						\
		C.set_pos(int(CPTR - LITS));									\


	// apply hyper binary resolution
	// 'clause size' must be read before calling 'newHyper2'
	#define HYPER_BINARY(ENABLED,LITS,SIZE,REF,OTHER,FLIPPED,POSTPTR)	\
		if (ENABLED) {													\
			const uint32 DOM = hyper2Resolve(LITS, SIZE, OTHER);		\
			if (DOM) {													\
				CHECKLIT(DOM);											\
				LOG2(4, "  adding hyper binary resolvent(%d %d)",		\
						l2i(DOM), l2i(OTHER));							\
				assert(learntC.empty());								\
				learntC.push(DOM);										\
				learntC.push(OTHER);									\
				if (opts.proof_en) proof.addClause(learntC);			\
				newHyper2();											\
				DELAY_WATCH(FLIPPED, OTHER, REF, SIZE);					\
				POSTPTR--;												\
			}															\
		}																\


	#define SWAP_WATCHES(LITS, CPTR, OTHER, NEWLIT, FLIPPED)			\
	{																	\
		*LITS = OTHER;													\
		*(LITS + 1) = NEWLIT;											\
		*CPTR = FLIPPED;												\
	}																	\


	#define RECOVER_WATCHES(WS,END,PREPTR,POSTPTR)						\
	{																	\
		while (PREPTR != END)											\
			*POSTPTR++ = *PREPTR++;										\
		WS.resize(uint32(POSTPTR - WS));								\
	}																	\

}

#endif