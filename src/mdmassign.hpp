/***********************************************************************[mdmassign.hpp]
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

#ifndef __MDM_ASSIGN_
#define __MDM_ASSIGN_

#include "solvetypes.hpp"

namespace SeqFROST {

	#define MDM_ASSUME (opts.mdmassume_en && assumptions.size())

	#define mdm_prefetch(MAXOCCURS,VALUES,STATES,OCCURS,FROZEN,CS,TAIL) \
		const uint32 MAXOCCURS = opts.mdm_max_occs;		\
		const LIT_ST* VALUES = sp->value;				\
		const State_t* STATES = sp->state;				\
		const OCCUR* OCCURS = occurs.data();			\
		const cbucket_t* CS = cm.address(0);			\
		LIT_ST* FROZEN = sp->frozen;					\
		sp->stacktail = sp->tmpstack;					\
		uint32*& TAIL = sp->stacktail;					\

	#define mdm_assign(CAND,DEC) \
		assert(CAND == ABS(DEC)); \
		WL& ws = wt[DEC]; \
		if (valid(values, cs, ws) && depFreeze(CAND, cs, values, frozen, tail, ws)) { \
			enqueueDecision(DEC); \
			sp->seen[CAND] = 1; \
		} \

}

#endif