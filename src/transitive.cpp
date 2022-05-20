/***********************************************************************[transitive.cpp]
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

void Solver::transitive()
{
	if (UNSAT) return;
	if (!opts.transitive_en) return;
	assert(probed);
	assert(!LEVEL);
	assert(sp->propagated == trail.size());
	sortWT();
	SET_BOUNDS(limit, transitive, transitiveticks, searchticks, 0);
	assert(last.transitive.literals < inf.nDualVars);
	PREFETCH_CM(cs, deleted);
	const State_t* states = sp->state;
	const LIT_ST* values = sp->value;
	LIT_ST* marks = sp->marks;
	uVec1D& marked = minimized;
	uint32 tried = 0, units = 0;
	uint64 removed = 0;
	while (stats.transitiveticks <= limit
		&& last.transitive.literals < inf.nDualVars
		&& NOT_UNSAT && !INTERRUPTED) {
		const uint32 src = last.transitive.literals++;
		CHECKLIT(src);
		const uint32 srcvar = ABS(src);
		if (!states[srcvar].state && UNASSIGNED(values[src])) {
			tried++;
#ifdef LOGGING
			LOG2(4, "  performing transitive reduction on literal %d", l2i(src));
#endif

			bool failed = false;
			WL& sws = wt[src];
			stats.transitiveticks += CACHELINES(sws.size()) + 1;
			forall_watches(sws, i) {
				assert(!failed);
				const WATCH sw = *i;
				if (!sw.binary()) break;
				const C_REF cref = sw.ref;
				if (deleted[cref]) continue;
				const uint32 dest = sw.imp;
				CHECKLIT(dest);
				if (!UNASSIGNED(values[dest])) continue;
				GET_CLAUSE(c, cref, cs);
				LOGCLAUSE(4, c, "  finding a transitive path to %d using", l2i(dest));
				const bool learnt = c.learnt();
				assert(marked.empty());
				assert(UNASSIGNED(l2marker(src)));
				marks[srcvar] = SIGN(src);
				marked.push(src);
				bool transitive = false;
				uint32 propagated = 0;
				while (!transitive && !failed && propagated < marked.size()) {
					const uint32 assign = marked[propagated++];
					CHECKLIT(assign);
					assert(l2marker(assign) == SIGN(assign));

#ifdef LOGGING
					LOG2(4, "  transitively propagating %d in:", l2i(assign));
#endif	

					WL& aws = wt[assign];
					stats.transitiveticks += CACHELINES(aws.size()) + 1;
					forall_watches(aws, j) {
						const WATCH aw = *j;
						if (!aw.binary()) break;
						const C_REF dref = aw.ref;
						if (dref == cref) continue;
						if (deleted[dref]) continue;
						GET_CLAUSE(d, dref, cs);
						if (!learnt && d.learnt()) continue;
						LOGCLAUSE(4, d, "  ");
						const uint32 other = aw.imp;
						CHECKLIT(other);
						if (other == dest) { 
							transitive = true; 
							break;
						}
						else {
							const uint32 othervar = ABS(other);
							const LIT_ST marker = marks[othervar];
							if (UNASSIGNED(marker)) {
								marks[othervar] = SIGN(other);
								marked.push(other);
							}
							else if (NEQUAL(marker, SIGN(other))) { 
								failed = true;
								break;
							}						
						}
					}
				}
				forall_vector(uint32, marked, i) { 
					marks[ABS(*i)] = UNDEF_VAL;
				}
				marked.clear();
				if (transitive) {
					LOGCLAUSE(4, c, "  found transitive clause");
					removeClause(c, cref);
					removed++;
				}
				if (failed) break;
				if (stats.transitiveticks > limit) break;
			}
			if (failed) {
				units++;
				LOG2(4, "  found failed literal %d during transitive reduction", l2i(src));
				enqueueUnit(FLIP(src));
				if (BCP()) {
					LOG2(2, " Propagation within transitive reduction proved a contradiction");
					learnEmpty();
				}
			}
		}
	}
	LOG2(2, " Transitive %lld: tried %d literals, removing %lld clauses and %d units",
		stats.probe.calls, tried, removed, units);
	if (last.transitive.literals == inf.nDualVars)
		last.transitive.literals = 2;
	stats.transitive.probed += tried;
	stats.transitive.failed += units;
	stats.transitive.removed += removed;
	printStats((units || removed), 't', CVIOLET3);
}
