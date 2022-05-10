/***********************************************************************[map.cpp]
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

#include "control.hpp"
#include "solve.hpp"

using namespace SeqFROST;

void Solver::map(const bool& sigmified)
{
	assert(inf.unassigned);
	assert(conflict == UNDEF_REF);
	assert(!LEVEL);
	assert(trail.size() == sp->propagated);
	stats.mapping.calls++;
	vmap.initiate(sp);
	// map model literals
	vmap.mapOrgs(model.lits);
	// map assumptions & frozen
	assert(iconflict.empty());
	if (assumptions.size()) 
		vmap.mapOrgs(assumptions);
	if (ifrozen.size()) 
		vmap.mapShrinkVars(ifrozen);
	// map transitive start literal
	vmap.mapTransitive(last.transitive.literals);
	// map clauses and watch tables
	if (!sigmified) {
		vmap.mapClauses(cm, orgs);
		vmap.mapClauses(cm, learnts);
		vmap.mapWatches(wt);
	}
	else {
		mapped = true;
		newBeginning();
		mapped = false;
	}
	// map trail, queue and heap
	vmap.mapShrinkLits(trail);
	sp->propagated = trail.size();
	const uint32 firstFrozen = vmap.firstL0();
	// - VMTF
	vmtf.map(*vmap, firstFrozen, vmap.size());
	vmap.mapShrinkVars(bumps);
	// - VSIDS
	uVec1D tmp;
	while (vsidsheap.size()) {
		uint32 x = vsidsheap.pop();
		if (x == firstFrozen) continue;
		uint32 mx = vmap.mapped(x);
		if (mx) tmp.push(mx);
	}
	vmap.mapShrinkVars(vsids.scores);
	vsidsheap.rebuild(tmp);
	// - CHB
	tmp.clear();
	while (chbheap.size()) {
		uint32 x = chbheap.pop();
		if (x == firstFrozen) continue;
		uint32 mx = vmap.mapped(x);
		if (mx) tmp.push(mx);
	}
	vmap.mapShrinkVars(chb.scores);
	vmap.mapShrinkVars(chb.conflicts);
	chbheap.rebuild(tmp);
	// shrink others without mapping
	assert(dlevel.size() == 1);
	eligible.clear(true);
	eligible.reserve(vmap.numVars());
	elected.clear(true);
	elected.reserve(vmap.numVars());
	occurs.clear(true);
	dlevel.clear(true);
	dlevel.reserve(vmap.numVars());
	dlevel.push(level_t());
	// map search space
	SP* newSP = new SP(vmap.size(), opts.polarity);
	vmap.mapSP(newSP);
	delete sp;
	sp = newSP;
	// map model and proof variables
	vmap.mapShrinkVars(vorg); 
	model.init(vorg);
	if (opts.proof_en) 
		proof.init(sp, vorg);
	// update phase-saving counters
	sp->trailpivot = 0, last.rephase.best = last.rephase.target = 0;
	for (uint32 v = 1; v <= vmap.numVars(); ++v) {
		if (sp->state[v].state) continue;
		Phase_t& ph = sp->phase[v];
		if (!PHASE_UNASSIGNED(ph.best)) last.rephase.best++;
		if (!PHASE_UNASSIGNED(ph.target)) last.rephase.target++;
	}

	stats.mapping.compressed += inf.maxVar - vmap.numVars();
	LOG2(2, " Variable mapping compressed %d to %d", inf.maxVar, vmap.numVars());

	inf.maxVar = vmap.numVars();
	inf.nDualVars = V2L(inf.maxVar + 1);
	inf.maxFrozen = inf.maxMelted = inf.maxSubstituted = 0;
	vmap.destroy();
	printStats(1, 'a', CGREEN2);
}