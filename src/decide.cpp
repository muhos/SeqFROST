/***********************************************************************[decide.cpp]
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

uint32 Solver::nextHeap(dheap_t& heap)
{
	assert(inf.unassigned);
	State_t* states = sp->state;
	const LIT_ST* values = sp->value;
	uint32 cand = 0;
	while (heap.size()) {
		cand = heap.top();
		CHECKVAR(cand);
		if (!states[cand].state && UNASSIGNED(values[V2L(cand)])) break;
		heap.pop();
	}

	assert(cand);
	LOG2(4, " Next heap choice %d, score %e", 
		cand, decheuristic == 0 ? vsids.scores[cand] : chb.scores[cand]);

	mab.decisions++;
	if (!states[cand].mab) {
		mab.chosen++;
		states[cand].mab = 1;
	}

	return cand;
}

uint32 Solver::nextQueue()
{
	assert(inf.unassigned);
	const State_t* states = sp->state;
	const LIT_ST* values = sp->value;
	uint32 free = vmtf.free();
	assert(free);
	if (states[free].state || !UNASSIGNED(values[V2L(free)])) {
		do { 
			free = vmtf.previous(free);
		} 
		while (states[free].state || !UNASSIGNED(values[V2L(free)]));
		vmtf.update(free, bumps[free]);
	}
	LOG2(4, " Next queue choice %d, bumped %lld", free, bumps[free]);
	return free;
}

uint32 Solver::makeAssign(const uint32& v, const bool& tphase) 
{
	CHECKVAR(v);
	LIT_ST pol = UNDEF_VAL;
	const Phase_t& ph = sp->phase[v];
	if (tphase) pol = ph.target;
	if (UNASSIGNED(pol)) pol = ph.saved;
	assert(pol >= 0);
	return V2DEC(v, pol);
}

void Solver::decide()
{
	assert(inf.unassigned);
	assert(sp->propagated == trail.size());
	assert(conflict == UNDEF_REF);
	assert(UNSOLVED);
	uint32 cand = stable ? nextHeap(DECISIONHEAP) : nextQueue();
	uint32 dec = makeAssign(cand, TARGETING);
	enqueueDecision(dec);
	stats.decisions.single++;
}