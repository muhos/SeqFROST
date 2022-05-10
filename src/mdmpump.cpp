/***********************************************************************[mdmpump.cpp]
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


inline void Solver::pumpFrozenHeap(const uint32& lit)
{
	CHECKLIT(lit);
	WL& ws = wt[lit];
	if (ws.empty()) return;
	uint32 v = ABS(lit);
	assert(!sp->frozen[v]);
	const score_t& heapActivity = HEAPSCORES;
	double norm_act = (double)sp->level[v] / last.mdm.decisions;
	forall_watches(ws, w) {
		if (cm.deleted(w->ref)) continue;
		uint32 frozen_v;
		if (w->binary()) frozen_v = ABS(w->imp);
		else {
			CLAUSE& c = cm[w->ref];
			frozen_v = ABS(c[0]) ^ ABS(c[1]) ^ v;
			assert(frozen_v != v);
		}
		CHECKVAR(frozen_v);
		if (heapActivity[frozen_v] == 0) {
			if (decheuristic == 0) {
				vsids.scores[frozen_v] = norm_act;
				vsidsheap.update(frozen_v);
			}
			else {
				chb.scores[frozen_v] = norm_act;
				chbheap.update(frozen_v);
			}
		}
	}
}

inline void Solver::pumpFrozenQue(const uint32& lit)
{
	CHECKLIT(lit);
	WL& ws = wt[lit];
	if (ws.empty()) return;
	uint32 v = ABS(lit);
	assert(!sp->frozen[v]);
	forall_watches(ws, w) {
		if (cm.deleted(w->ref)) continue;
		uint32 frozen_v;
		if (w->binary()) frozen_v = ABS(w->imp);
		else {
			CLAUSE& c = cm[w->ref];
			frozen_v = ABS(c[0]) ^ ABS(c[1]) ^ v;
			assert(frozen_v != v);
		}
		CHECKVAR(frozen_v);
		if (sp->frozen[frozen_v]) {
			analyzed.push(frozen_v);
			sp->frozen[frozen_v] = 0;
		}
	}
}

void Solver::pumpFrozen()
{
	if (!last.mdm.decisions) return;

	assert(trail.size() > sp->propagated);

	LOGN2(2, " Pumping frozen variables..");

	uint32* start = trail + sp->propagated;

	if (stable && opts.mdm_heap_pumps) {

		for (uint32* assign = trail.end() - 1; assign >= start; --assign)
			pumpFrozenHeap(*assign);

		opts.mdm_heap_pumps--;
	}
	else if (opts.mdm_vmtf_pumps) { // VMFQ (requires special handling)

		assert(analyzed.empty());
		assert(inf.maxVar >= last.mdm.decisions);

		analyzed.reserve(inf.maxVar - last.mdm.decisions);

		for (uint32* assign = trail.end() - 1; assign >= start; --assign)
			pumpFrozenQue(*assign);

		if (analyzed.size()) {

			start = analyzed;

			for (uint32* v = analyzed.end() - 1; v >= start; --v)
				varBumpQueueNU(*v);

			const uint32 first = *start;
			if (UNASSIGNED(sp->value[V2L(first)])) 
				vmtf.update(first, bumps[first]);

			analyzed.clear(true);

			opts.mdm_vmtf_pumps--;
		}
	}

	LOGDONE(2, 4);
}