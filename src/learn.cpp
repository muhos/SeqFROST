/***********************************************************************[learn.cpp]
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

inline uint32 Solver::where()
{
	CHECKLEVEL(LEVEL);

	uint32 jmplevel = UNDEF_LEVEL;

	if (learntC.size() == 1) 
		jmplevel = 0;
	else if (learntC.size() == 2 || 
		(!opts.minimizebin_en && opts.minimizesort_en))
		jmplevel = l2dl(learntC[1]);
	else {
		assert(learntC.size() > 2);
		assert(LEVEL > 1);
		uint32 chronolevel = LEVEL - 1;
		uint32* lits = learntC;
		uint32* maxPos = lits + 1, maxLit = *maxPos, * end = learntC.end();
		jmplevel = l2dl(maxLit);
		for (uint32* k = lits + 2; k != end; ++k) {
			const uint32 lit = *k;
			const uint32 litLevel = l2dl(lit);
			if (jmplevel >= litLevel) continue;
			jmplevel = litLevel;
			maxLit = lit;
			maxPos = k;
			if (litLevel == chronolevel) break;
		}
		*maxPos = lits[1];
		lits[1] = maxLit;
		LOGLEARNT(this, 3);
	}

	assert(LEVEL > jmplevel);
	if (opts.chrono_en && ((LEVEL - jmplevel) > opts.chrono_min)) {
		jmplevel = LEVEL - 1;
		stats.backtrack.chrono++;
	}
	else 
		stats.backtrack.nonchrono++;

	CHECKLEVEL(jmplevel);
	return jmplevel;
}

C_REF Solver::learn()
{
	assert(trail.size());

	const uint32 jmplevel = where();

	backtrack(jmplevel);

	if (learntC.size() == 1) {
		enqueueUnit(learntC[0]);
		stats.units.learnt++;
	}
	else {
		if (opts.proof_en) proof.addClause(learntC);
		C_REF r = addClause(learntC, true);
		enqueue(*learntC, jmplevel, r);
		return r;
	}

	return UNDEF_REF;
}