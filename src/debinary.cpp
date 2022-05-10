/***********************************************************************[debins.cpp]
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
using namespace SeqFROST;

void Solver::debinary() 
{
	if (!opts.debinary_en) return;
	if (UNSAT) return;
	assert(!LEVEL);
	assert(!wt.empty());
	assert(sp->propagated == trail.size());

	stats.debinary.calls++;

	PREFETCH_CM(cs, deleted);

	const State_t* states = sp->state;
	LIT_ST* marks = sp->marks;
	uVec1D& marked = minimized;
	int64 subsumed = 0, units = 0;
	forall_literals(lit) {
		assert(marked.empty());
		if (states[ABS(lit)].state) continue;
		uint32 unit = 0;
		WL& ws = wt[lit];
		WATCH *j = ws;
		forall_watches(ws, i) {
			const WATCH w = *j++ = *i;
			const C_REF cref = w.ref;
			if (deleted[cref]) { j--; continue; }
			if (w.binary()) {
				const uint32 other = w.imp, othervar = ABS(other);
				CHECKLIT(other);
				const LIT_ST othersign = SIGN(other);
				const LIT_ST marker = marks[othervar];
				GET_CLAUSE(c, cref, cs);
				assert(c.size() == 2);
				if (UNASSIGNED(marker)) {
					marks[othervar] = othersign;
					marked.push(other);
				}
				else if (NEQUAL(marker, othersign)) { // found 'hyper unary'
					unit = FLIP(lit);
					j = ws; // the whole list is satisfied by 'unit'
					units++;
					break;
				}
				else { // found duplicate
					LOGCLAUSE(4, c, "  found duplicated binary");
					if (c.original()) { // find learnt duplicate if exists
						for (WATCH* k = ws; ; ++k) {
							assert(k != i);
							if (!k->binary() || NEQUAL(k->imp, other)) continue;
							const C_REF dref = k->ref;
							if (deleted[dref]) continue;
							GET_CLAUSE(d, dref, cs);
							assert(d.size() == 2);
							assert(!deleted[dref]);
							removeClause(d, dref);
							*k = w;
							break;
						}
					}
					else
						removeClause(c, cref);
					subsumed++;
					j--;
				}
			}
		}

		if (j != ws) 
			ws.resize(uint32(j - ws));
		else 
			ws.clear(true);

		forall_vector(uint32, marked, i) { 
			marks[ABS(*i)] = UNDEF_VAL; 
		}

		marked.clear();

		if (unit) {
			CHECKLIT(unit);
			enqueueUnit(unit);
			if (BCP()) { learnEmpty(); break; }
		}
	}
	stats.debinary.hyperunary += units;
	stats.debinary.binaries += subsumed;
	LOG2(2, " Deduplicate %lld: removed %lld binaries, producing %lld hyper unaries", stats.debinary.calls, subsumed, units);
	printStats(units || subsumed, 'd', CVIOLET2);
}