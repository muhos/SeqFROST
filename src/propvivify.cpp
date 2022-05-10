/***********************************************************************[propvivfy.cpp]
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

#include "solve.hpp"
#include "prophelper.hpp"

using namespace SeqFROST;

bool Solver::BCPVivify() 
{
	assert(UNSOLVED);

	conflict = UNDEF_REF;

	// as long as we don't add new clauses to 'cm'
	// prefetching here is safe
	PREFETCH_CM(cs, deleted);
	
	bool isConflict = false;
	while (!isConflict && sp->propagated < trail.size()) {
		isConflict = propVivify(trail[sp->propagated++], cs, deleted);
	}
	return isConflict;
}

inline bool Solver::propVivify(const uint32& assign, const cbucket_t* cs, const bool* deleted)
{
	CHECKLIT(assign);
	const uint32 level = l2dl(assign);
	const uint32 flipped = FLIP(assign);
#ifdef LOGGING
	LOG2(4, "  propagating %d@%d in large clauses", l2i(assign), level);
#endif
	const LIT_ST* values = sp->value;
	WL& ws = wt[assign];
	uint64 ticks = CACHELINES(ws.size()) + 1;
	WATCH* i = ws, * j = i, * wend = ws.end();
	while (i != wend) {
		
		ADVANCE_WATCHES(w, imp, impval, ref, i, j, values);

		if (w.binary()) {

			if (deleted[ref]) { j--; continue; } 

			if (impval) 
				enqueue(imp, level, ref);
			else { 
				conflict = ref; 
				break; 
			}
		}
		else if (NEQUAL(ref, ignore)) {
			ticks++;

			if (deleted[ref]) { j--; continue; }

			PREFETCH_LARGE_CLAUSE(c, lits, other, otherval, ref, flipped, values, cs);

			if (otherval > 0)
				(j - 1)->imp = other;
			else {

				const int csize = c.size();

				FIND_NEW_WATCH(c, lits, c.pos(), csize, k, newlit, values);

				LIT_ST val = values[newlit];

				if (val > 0)
					(j - 1)->imp = newlit;
				else if (UNASSIGNED(val)) {
					
					SWAP_WATCHES(lits, k, other, newlit, flipped);

					DELAY_WATCH(newlit, other, ref, csize);

					j--;

					ticks++;
				}
				else if (UNASSIGNED(otherval)) {
					assert(!val);
					enqueue(other, forcedLevel(other, c), ref);
				}
				else {
					assert(!val);
					assert(!otherval);
					LOGCONFLICT(3, other);
					conflict = ref;
					break;
				}
			}
		}
	} // end of watches loop
	
	RECOVER_WATCHES(ws, wend, i, j);

	REATTACH_DELAYED;

	stats.probeticks += ticks;

	return NEQUAL(conflict, UNDEF_REF);
}