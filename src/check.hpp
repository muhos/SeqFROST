/***********************************************************************[check.hpp]
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

#ifndef __CHECK_
#define __CHECK_

#include "vector.hpp"
#include "datatypes.hpp"
#include "logging.hpp"

namespace SeqFROST {

	template <class T>
	inline bool  _checkvar(const T& VAR, const T& MAXVAR) {
		const bool invariant = VAR <= 0 || VAR > MAXVAR;
		if (invariant)
			LOGERRN("invariant \"VAR > 0 && VAR <= inf.maxVar\" failed on variable (%lld), bound = %lld", 
				int64(VAR), int64(MAXVAR));
		return !invariant;
	}

	template <class T>
	inline bool  _checklit(const T& LIT, const T& MAXLIT) {
		const bool invariant = LIT <= 1 || LIT >= MAXLIT;
		if (invariant)
			LOGERRN("invariant \"LIT > 1 && LIT < inf.nDualVars\" failed on literal (%lld), bound = %lld",
				int64(LIT), int64(MAXLIT));
		return !invariant;
	}

	template <class T>
	inline bool  _checklvl(const T& LVL, const T& MAXLEVEL) {
		const bool invariant = LVL == MAXLEVEL;
		if (invariant)
			LOGERRN("invariant \"LEVEL == UNDEF_LEVEL\" failed on level (%lld), bound = %lld",
				int64(LVL), int64(MAXLEVEL));
		return !invariant;
	}

	template <class HEAPTYPE, class SCORETYPE>
	bool _checkdecisionheap(HEAPTYPE& dheap, const SCORETYPE& scores)
	{
		const uint32* pos	  = dheap.indices();
		const uint32* heap	  = dheap.data();
		const uint32  size    = dheap.size();
		for (uint32 i = 0; i < size; i++) {
			const uint32 x = heap[i];
			const uint32 xpos = pos[x];
			if (i != xpos) {
				LOGERRN("heap index(%d) is different from heap stored position %d", i, xpos);
				return false;
			}
			uint32 childpos = (xpos << 1) + 1;
			uint32 parentpos = (childpos - 1) >> 1;
			assert(parentpos == xpos);
			if (parentpos != xpos) {
				LOGERRN("parent index(%d) and heap position(%d) are inconsistent", parentpos, xpos);
				return false;
			}
			if (childpos < size) {
				uint32 child = heap[childpos];
				if (scores[x] < scores[child]) {
					LOGERRN("score[%d] = %g cannot be less than its child score[%d] = %g", x, scores[x], child, scores[child]);
					return false;
				}
				uint32 otherpos = childpos + 1;
				if (otherpos < size) {
					parentpos = (otherpos - 1) >> 1;
					if (parentpos != xpos) {
						LOGERRN("other parent index(%d) and heap position(%d) are inconsistent", parentpos, xpos);
						return false;
					}
					child = heap[otherpos];
					if (scores[x] < scores[child]) {
						LOGERRN("score[%d] = %g cannot be less than its other child score[%d] = %g", x, scores[x], child, scores[child]);
						return false;
					}
				}
			}
		}
		return true;
	}

	inline bool	_checkMDM(const LIT_ST* frozen, const uVec1D& trail, const uint32& propagated) 
	{
		for (uint32 i = propagated; i < trail.size(); ++i) {
			uint32 v = ABS(trail[i]);
			if (frozen[v]) {
				LOG0("");
				LOGERRN("decision(%d) is elected and frozen", v);
				return false;
			}
		}
		return true;
	}

	inline bool	_checkseen(const LIT_ST* seen, const uint32& MAXVAR)
	{
		for (uint32 v = 0; v <= MAXVAR; ++v) {
			if (seen[v]) {
				LOG0("");
				LOGERRN("seen(%d) is not unseen", v);
				return false;
			}
		}
		return true;
	}

	inline bool	_checkfrozen(const LIT_ST* frozen, const uint32& MAXVAR) 
	{
		for (uint32 v = 0; v <= MAXVAR; ++v) {
			if (frozen[v]) {
				LOG0("");
				LOGERRN("frozen(%d) is not melted", v);
				return false;
			}
		}
		return true;
	}
	
	inline bool	_verifyelected(const uVec1D& elected, const LIT_ST* frozen) {
		for (uint32 i = 0; i < elected.size(); ++i)
			if (frozen[elected[i]])
				return false;
		return true;
	}

	inline bool _verifymarkings(Vec<LIT_ST>& marks, const Lits_t& clause)
	{
		for (int k = 0; k < clause.size(); ++k) {
			const uint32 orglit = clause[k];
			assert(orglit > 1 && orglit < UNDEF_VAR);
			const uint32 orgvar = ABS(orglit);
			while (orgvar >= marks.size())
				marks.push(UNDEF_VAL);
			if (marks[orgvar] != UNDEF_VAL)
				return false;
		}
		return true;
	}
}

#endif 

