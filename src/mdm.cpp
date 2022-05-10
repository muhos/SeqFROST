/***********************************************************************[mdm.cpp]
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
#include "mdmrank.hpp"
#include "mdmlimit.hpp"
#include "mdmassign.hpp"

using namespace SeqFROST;

inline bool Solver::valid(const LIT_ST* values, const cbucket_t* cs, WL& ws)
{
	forall_watches(ws, i) {
		const WATCH w = *i;
		// clause satisfied
		if (values[w.imp] > 0) continue; 
		// if 'w.imp' not satisfied then it's an implication of 'cand'
		if (w.binary()) 
			return false; 
		// there cannot be falsified literal as watched,
		// so validating starts from 'c + 2'
		GET_CLAUSE(c, w.ref, cs);
		assert(c.size() > 2);
		bool satisfied = false, unAssigned = false;
		uint32* k = c + 2, * cend = c.end();
		while (k != cend && !satisfied && !unAssigned) {
			CHECKLIT(*k);
			const LIT_ST val = values[*k];
			if (UNASSIGNED(val)) unAssigned = true;
			else if (val) satisfied = true;
			k++;
		}
		if (!satisfied && !unAssigned) 
			return false;
	}
	return true;
}

inline bool Solver::depFreeze(const uint32& cand, const cbucket_t* cs, const LIT_ST* values, LIT_ST* frozen, uint32*& stack, WL& ws)
{
	forall_watches(ws, i) {
		const WATCH w = *i;
		if (values[w.imp] > 0) continue;
		assert(!w.binary());
		GET_CLAUSE(c, w.ref, cs);
		uint32* lits = c.data();
		uint32 othervar = ABS(lits[0]) ^ ABS(lits[1]) ^ cand;
		if (sp->seen[othervar]) return false;
		if (!frozen[othervar]) {
			frozen[othervar] = 1;
			assert(stack < sp->tmpstack + inf.maxVar);
			*stack++ = othervar;
		}
		uint32* k = lits + 2, * cend = c.end();
		while (k != cend) {
			othervar = ABS(*k++);
			if (sp->seen[othervar]) return false;
			if (!frozen[othervar]) {
				frozen[othervar] = 1;
				assert(stack < sp->tmpstack + inf.maxVar);
				*stack++ = othervar;
			}
		}
	}
	return true;
}

inline void Solver::MDMAssume(const LIT_ST* values, const cbucket_t* cs, LIT_ST* frozen, uint32*& tail)
{
	assert(sp->stacktail == sp->tmpstack);
	uint32 level = LEVEL;
	while (level < assumptions.size()) {
		const uint32 a = assumptions[level];
		CHECKLIT(a);
		assert(ifrozen[ABS(a)]);
		const uint32 cand = ABS(a);
		const LIT_ST val = values[a];
		if (UNASSIGNED(val)) {
			level++;
			mdm_assign(cand, a);
		}
		else if (!val) {
			ianalyze(FLIP(a));
			SET_UNSAT;
			clearMDM();
			return;
		}
	}
	stats.decisions.massumed += trail.size() - sp->propagated;
}

void Solver::MDMInit()
{
	if (!last.mdm.rounds) return;

	assert(inf.unassigned);
	assert(sp->propagated == trail.size());
	assert(conflict == UNDEF_REF);
	assert(UNSOLVED);

	stats.mdm.calls++;

	LOG2(2, " MDM %d: electing decisions at decaying round %d..", stats.mdm.calls, last.mdm.rounds);

	if (opts.mdm_walk_init_en) {
		stats.mdm.walks++;
		walk();
	}

	eligible_initial; 

	mdm_prefetch(maxoccurs, values, states, occs, frozen, cs, tail);

	if (MDM_ASSUME)
		MDMAssume(values, cs, frozen, tail);

	forall_vector(uint32, eligible, evar) {
		const uint32 cand = *evar;
		CHECKVAR(cand);
		if (frozen[cand] || states[cand].state || iassumed(cand)) continue;
		if (occs[cand].ps > maxoccurs || occs[cand].ns > maxoccurs) continue;
		const uint32 dec = V2DEC(cand, sp->phase[cand].saved);
		mdm_assign(cand, dec);
	}

	mdm_update;

	if (opts.mdm_heap_pumps || opts.mdm_vmtf_pumps)
		pumpFrozen();

	clearMDM();

	printStats(1, 'm', CMDM);
}

void Solver::MDM()
{
	assert(inf.unassigned);
	assert(sp->propagated == trail.size());
	assert(conflict == UNDEF_REF);
	assert(UNSOLVED);

	stats.mdm.calls++;

	LOG2(2, " MDM %d: electing decisions at decaying round %d..", stats.mdm.calls, last.mdm.rounds);

	if (stable) {
		if (decheuristic == 0) {
			eligible_heap(vsids);
		}
		else {
			eligible_heap(chb);
		}
	}
	else
		eligible_queue(vmtf);

	assert(eligible.size() >= 1);

	const bool firstround = last.mdm.rounds == opts.mdm_rounds;
	if (opts.mdm_walk_en && !LEVEL && firstround) {
		stats.mdm.walks++;
		walk();
	}

	mdm_prefetch(maxoccurs, values, states, occs, frozen, cs, tail);

	if (MDM_ASSUME)
		MDMAssume(values, cs, frozen, tail);

	const bool targeting = TARGETING;
	forall_vector(uint32, eligible, evar) {
		const uint32 cand = *evar;
		CHECKVAR(cand);
		if (frozen[cand] || states[cand].state || iassumed(cand)) continue;
		if (stable && (occs[cand].ps > maxoccurs || occs[cand].ns > maxoccurs)) continue;
		uint32 dec = makeAssign(cand, targeting);
		mdm_assign(cand, dec);
	}

	mdm_update;

	if (opts.mdm_heap_pumps || opts.mdm_vmtf_pumps) 
		pumpFrozen();

	clearMDM();

	printStats(firstround, 'm', CMDM);
}