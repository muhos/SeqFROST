/***********************************************************************[propelim.cpp]
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

#include "simplify.hpp" 

using namespace SeqFROST;

inline bool Solver::propClause(const LIT_ST* values, const uint32& lit, SCLAUSE& c)
{
	uint32 sig = 0;
	uint32* j = c;
	forall_clause(c, i) {
		const uint32 other = *i;
		if (NEQUAL(other, lit)) {
			if (values[other] > 0) return true;
			*j++ = other;
			sig |= MAPHASH(other);
		}
		else
			assert(!values[other]);
		
	}
	assert(int(j - c) == c.size() - 1);
	assert(c.hasZero() < 0);
	c.set_sig(sig);
	c.pop();
	assert(c.isSorted());
	return false;
}

bool Solver::prop()
{
	numforced = sp->propagated;
	LIT_ST* values = sp->value;
	while (sp->propagated < trail.size()) { 
		const uint32 assign = trail[sp->propagated++], flipped = FLIP(assign);
		CHECKLIT(assign);
		// reduce unsatisfied
		OL& fot = ot[flipped];
		forall_occurs(fot, i) {
			SCLAUSE& c = scnf[*i];
			assert(c.size());
			if (c.deleted()) continue;
			if (propClause(values, flipped, c))
				c.markDeleted(); // clause satisfied by an assigned unit
			else {
				const int size = c.size();
				if (!size) { learnEmpty(); return false; }
				if (size == 1) {
					const uint32 unit = *c;
					CHECKLIT(unit);
					if (UNASSIGNED(values[unit])) enqueueUnit(unit);
					else { learnEmpty(); return false; }
				}
			}
		}
		fot.clear(true);
		toblivion(scnf, ot[assign]);
	}
	numforced = sp->propagated - numforced;
	if (numforced) {
		LOGREDALL(this, 2, "BCP Reductions");
		numforced = 0;
		if (!opts.sub_en) reduceOT();
	}
	return true;
}