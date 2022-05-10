/***********************************************************************[elimination.cpp]
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

#include "simplify.hpp" 

using namespace SeqFROST;

void Solver::BCE()
{
	if (opts.bce_en) {
		if (INTERRUPTED) killSolver();
		LOG2(2, " Eliminating blocked clauses..");
		if (opts.profile_simplifier) timer.pstart();
		forall_vector(uint32, elected, i) {
			const uint32 v = *i;
			if (!v) continue;
			const uint32 p = V2L(v), n = NEG(p);
			OL& poss = ot[p], &negs = ot[n];
			if (poss.size() <= opts.bce_max_occurs && negs.size() <= opts.bce_max_occurs) {
				// start with negs
				forall_occurs(negs, i) {
					SCLAUSE& neg = scnf[*i];
					if (neg.original()) {
						bool allTautology = true;
						forall_occurs(poss, j) {
							SCLAUSE& pos = scnf[*j];
							if (pos.original() && !isTautology(v, neg, pos)) {
								allTautology = false;
								break;
							}
						}
						if (allTautology) {
							assert(neg.original());
							model.saveClause(neg, neg.size(), n);
							neg.markDeleted();
						}
					}
				}
			}
		}
		if (opts.profile_simplifier) timer.pstop(), timer.bce += timer.pcpuTime();
		LOGREDALL(this, 2, "BCE Reductions");
	}
}