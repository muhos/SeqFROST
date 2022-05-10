/***********************************************************************[histogram.cpp]
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
#include "histogram.hpp"

using namespace SeqFROST;

inline bool Solver::isBinary(const C_REF& r, uint32& first, uint32& second)
{
	assert(!LEVEL);
	CLAUSE& c = cm[r];
	assert(!cm.deleted(r));
	first = 0, second = 0;
	forall_clause(c, k) {
		const uint32 lit = *k;
		CHECKLIT(lit);
		const LIT_ST val = sp->value[lit];
		if (UNASSIGNED(val)) {
			if (second) return false; // not binary
			if (first) second = lit;
			else first = lit;
		}
		else if (val) {
			// satisfied
			removeClause(c, r);
			return false;
		}
	}
	if (!second) return false; // all falsified except 'first'
	return true;
}

void Solver::histBins(BCNF& cnf)
{
	uint32* hist = vhist.data();
	const bool* deleted = cm.stencil();
	forall_cnf(cnf, i) {
		const C_REF ref = *i;
		if (deleted[ref]) continue;
		uint32 a, b;
		if (isBinary(ref, a, b)) {
			CHECKLIT(a), CHECKLIT(b);
			hist[a]++;
			hist[b]++;
		}
	}
}

void Solver::histCNF(BCNF& cnf, const bool& reset) 
{
	if (cnf.empty()) return;

	OCCUR* occs = occurs.data();

	PREFETCH_CM(cs, deleted);

	if (reset) 
		memset(occs, 0, occurs.size() * sizeof(OCCUR));

	forall_cnf(cnf, i) {
		const C_REF ref = *i;
		if (deleted[ref]) continue;
		GET_CLAUSE(c, ref, cs);
		count_occurs(c, occs);
	}
}

void Solver::histCNF(SCNF& cnf, const bool& reset) 
{
	if (cnf.empty()) return;

	OCCUR* occs = occurs.data();

	if (reset) 
		memset(occs, 0, occurs.size() * sizeof(OCCUR));

	forall_sclauses(scnf, i) {
		SCLAUSE& c = scnf[*i];
		if (c.deleted()) continue;
		count_occurs(c, occs);
	}
}