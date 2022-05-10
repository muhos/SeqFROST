/***********************************************************************[propsearch.cpp]
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

bool Solver::BCP()
{
	conflict = UNDEF_REF;

	// prefetch main pointers
	const uint32 propsbefore = sp->propagated;
	const LIT_ST* values = sp->value;

	PREFETCH_CM(cs, deleted);

	bool isConflict = false;

	while (!isConflict && sp->propagated < trail.size()) {
		const uint32 assign = trail[sp->propagated++];
		const uint32 flipped = FLIP(assign);
		const uint32 level = l2dl(assign);
		CHECKLIT(assign);
		CHECKLEVEL(level);
	#ifdef LOGGING
		LOG2(4, "  propagating %d@%d", l2i(assign), level);
	#endif
		WL& ws = wt[assign];
		uint64 ticks = CACHELINES(ws.size()); // 64-byte cache line is assumed
		WATCH* i = ws, *j = i, * wend = ws.end();
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
			else {
				ticks++;

				if (deleted[ref]) { j--; continue; }

				PREFETCH_LARGE_CLAUSE(c, lits, other, otherval, ref, flipped, values, cs);

				if (otherval > 0) 
					(j - 1)->imp = other;				// satisfied, replace "w.imp" with new blocking "other"
				else {
					
					const int csize = c.size();

					FIND_NEW_WATCH(c, lits, c.pos(), csize, k, newlit, values);

					LIT_ST val = values[newlit];

					if (val > 0)						// found satisfied new literal (update "imp")
						(j - 1)->imp = newlit; 
					else if (UNASSIGNED(val)) {			// found new unassigned literal to watch
						
						SWAP_WATCHES(lits, k, other, newlit, flipped);

						ATTACH_WATCH(newlit, other, ref, csize);

						j--;							// remove j-watch from current assignment

						ticks++;
					}
					else if (UNASSIGNED(otherval)) {	// clause is unit
						assert(!val);
						enqueue(other, forcedLevel(other, c), ref);
					}
					else {								// clause is conflicting
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

		stats.searchticks += ticks;

		isConflict = NEQUAL(conflict, UNDEF_REF);
	} // end of trail loop

	stats.searchprops += sp->propagated - propsbefore;

	if (isConflict) sp->trailpivot = dlevel.back().depth;
	else			sp->trailpivot = sp->propagated;

	// CHB bump
	const uint32 trailsize = trail.size();
	if (stable && LEVEL && trailsize && decheuristic == 1) { 
		const uint64 conflicts = stats.conflicts;
		const double multiplier = isConflict ? 1.0 : 0.9;
		const uint32* levels = sp->level;
		const uint32* assigns = trail.data();
		const uint32* tail = assigns + trailsize - 1;
		uint32 lit = *tail, v = ABS(lit);
		CHECKLIT(lit);
		while (tail >= assigns && levels[v] == LEVEL) {
			lit = *tail--, v = ABS(lit);
            CHECKLIT(lit);
			chb.bump(chbheap, v, conflicts, multiplier); 
		}
	}
	// CHB decay
	chb.decay(stable && isConflict && decheuristic == 1);

	return isConflict;
}