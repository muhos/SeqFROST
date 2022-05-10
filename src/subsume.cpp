/***********************************************************************[subsume.cpp]
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

#include "subsume.hpp" 

using namespace SeqFROST;

void Solver::SUB()
{
	if (opts.sub_en || opts.ve_plus_en) {
		if (INTERRUPTED) killSolver();
		LOG2(2, " Eliminating (self)-subsumptions..");
		if (opts.profile_simplifier) timer.pstart();
		const int maxoccurs = opts.sub_max_occurs;
		const int clausemax = opts.sub_clause_max;
		SUBSTATS& substats = stats.inprocess.sub;
		forall_vector(uint32, elected, i) {
			CHECKVAR(*i);
			assert(!sp->state[*i].state);
			const uint32 p = V2L(*i), n = NEG(p);
			OL& poss = ot[p], &negs = ot[n];
			if (poss.size() <= maxoccurs && negs.size() <= maxoccurs)
				self_sub_x(p, clausemax, scnf, poss, negs, substats);
		}
		if (opts.profile_simplifier) timer.pstop(), timer.sub += timer.pcpuTime();
		LOGREDALL(this, 2, "SUB Reductions");
	}
}

void Solver::strengthen(SCLAUSE& c, const uint32& me)
{
	uint32 sig = 0;
	uint32* j = c;
	forall_clause(c, k) {
		const uint32 lit = *k;
		if (NEQUAL(lit, me)) {
			*j++ = lit;
			sig |= MAPHASH(lit);
		}
	}
	assert(c.hasZero() < 0);
	c.set_sig(sig);
	c.pop();
	if (c.size() == 1) {
		const uint32 unit = *c;
		const LIT_ST val = sp->value[unit];
		if (UNASSIGNED(val))
			enqueueUnit(unit);
		else if (!val) { 
			LOG2(2, "  Subsume proved a contradiction");
			learnEmpty();
			killSolver();
		}
	}
	else {
		assert(c.isSorted());
		if (opts.proof_en) 
			proof.addResolvent(c);
		if (c.learnt()) 
			bumpShrunken(c);
	}
}