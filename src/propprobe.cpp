/***********************************************************************[propprobe.cpp]
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

bool Solver::BCPProbe() 
{
	assert(UNSOLVED);
	assert(LEVEL == 1);

	conflict = UNDEF_REF;

	bool isConflict = false;
	uint32 propagatedbin = sp->propagated;
	while (!isConflict && sp->propagated < trail.size()) {
		if (propagatedbin < trail.size())
			isConflict = propBinary(trail[propagatedbin++]);
		else
			isConflict = propProbe(trail[sp->propagated++]);
	}
	return isConflict;
}

inline bool Solver::propBinary(const uint32& assign)
{
	CHECKLIT(assign);
	assert(l2dl(assign) == 1);
#ifdef LOGGING
	LOG2(4, "  propagating %d@%d in binaries", l2i(assign), l2dl(assign));
#endif
	const LIT_ST* values = sp->value;
	WL& ws = wt[assign];
	forall_watches(ws, i) {
		const WATCH w = *i;
		if (w.binary()) {
			const uint32 imp = w.imp;
			CHECKLIT(imp);
			assert(imp != FLIP(assign));
			const LIT_ST impval = values[imp];
			if (impval > 0) continue; 
			const C_REF ref = w.ref;
			if (cm.deleted(ref)) continue;
			if (impval) 
				enqueue(imp, 1, ref);
			else { 
				conflict = ref; 
				return true;
			}
		}
	}
	return false;
}

inline bool Solver::propProbe(const uint32& assign)
{
	CHECKLIT(assign);
	assert(l2dl(assign) == 1);
#ifdef LOGGING
	LOG2(4, "  propagating %d@%d in large clauses", l2i(assign), l2dl(assign));
#endif
	const uint32 flipped = FLIP(assign);
	const LIT_ST* values = sp->value;
	WL& ws = wt[assign];
	uint64 ticks = CACHELINES(ws.size()) + 1;
	WATCH* i = ws, * j = i, * wend = ws.end();
	while (i != wend) {
		
		ADVANCE_WATCHES(w, imp, impval, ref, i, j, values);

		if (w.binary()) {
			if (cm.deleted(ref)) { j--; continue; }
			if (impval) 
				enqueue(imp, 1, ref);
			else { 
				conflict = ref; 
				break; 
			}
		}
		else if (NEQUAL(ref, ignore)) {
			ticks++;

			// 'cm' must be used as hyper binary may force cm to reallocate memory
			if (cm.deleted(ref)) { j--; continue; }

			GET_LARGE_CLAUSE(c, lits, other, otherval, ref, flipped, values);

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

					HYPER_BINARY(opts.probehbr_en, lits, csize, ref, other, flipped, j);

					enqueue(other, 1, ref);
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