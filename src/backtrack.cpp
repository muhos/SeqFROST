/***********************************************************************[backtrack.cpp]
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

inline void Solver::savePhases()
{
	const bool reset = (last.rephase.type && stats.conflicts > last.rephase.conflicts);
	if (reset) 
		last.rephase.target = 0;
	if (sp->trailpivot > last.rephase.target) {
		last.rephase.target = sp->trailpivot;
		sp->saveToTarget();
	}
	if (sp->trailpivot > last.rephase.best) {
		last.rephase.best = sp->trailpivot;
		sp->saveToBest();
	}
	sp->trailpivot = 0;
	if (reset) 
		last.rephase.type = 0;
}

inline void	Solver::cancelAssign(const uint32& lit) 
{
	CHECKLIT(lit);
	assert(inf.unassigned < inf.maxVar);

	sp->value[lit] = sp->value[FLIP(lit)] = UNDEF_VAL;
	inf.unassigned++;

#ifdef LOGGING
	LOG2(4, "  literal %d@%d cancelled", l2i(lit), l2dl(lit));
#endif
}

void Solver::backtrack(const uint32& jmplevel)
{
	CHECKLEVEL(LEVEL);
	CHECKLEVEL(jmplevel);

	if (LEVEL == jmplevel) return;

	const uint32 pivot = jmplevel + 1;
	const uint32 from = dlevel[pivot].depth;

	LOG2(3, " Backtracking to level %d, at trail position %d", jmplevel, from);

	if (!probed) savePhases();

	uint32* trailpos = sp->trailpos;
	const uint32* levels = sp->level;
	
	uint32* start = trail, *i = start + from, *j = i, *end = trail.end();
	if (stable) {
		dheap_t& heap = DECISIONHEAP;  
		while (i != end) {
			const uint32 lit = *i++, v = ABS(lit);
			if (levels[v] > jmplevel) {
				cancelAssign(lit);
				if (!heap.has(v)) 
					heap.insert(v);
			}
			else {
				trailpos[v] = j - start;
				*j++ = lit;
			}
		}
	}
	else {
		while (i != end) {
			const uint32 lit = *i++, v = ABS(lit);
			if (levels[v] > jmplevel) {
				cancelAssign(lit);
				if (vmtf.bumped() < bumps[v]) 
					vmtf.update(v, bumps[v]);
			}
			else {
				trailpos[v] = j - start;
				*j++ = lit;
			}
		}
	}
	const uint32 remained = uint32(j - start);
	LOG2(3, "  %d literals kept (%d are saved) and %zd are cancelled", remained, remained - from, end - j);
	trail.resize(remained);
	if (sp->propagated > from) sp->propagated = from;
	dlevel.resize(pivot);
	decisionlevel = jmplevel;
	assert(LEVEL == (dlevel.size() - 1));
}