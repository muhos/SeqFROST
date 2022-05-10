/***********************************************************************[lcve.cpp]
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
#include "sort.hpp"
#include "histogram.hpp"

namespace SeqFROST {
	uint32 orgcore[MAXFUNVAR] = { UNDEF_VAR };
}

using namespace SeqFROST;

bool Solver::LCVE()
{
	// reorder variables
	LOGN2(2, " Finding eligible variables for LCVE..");

	if (opts.profile_simplifier) timer.pstart();
	eligible.resize(inf.maxVar);
	occurs.resize(inf.maxVar + 1);
	histCNF(scnf, true);
	write_scores(vars, scores, occs);
	wolfsort(vars, inf.maxVar, STABLE_LCV(scores));
	if (opts.profile_simplifier) timer.pstop(), timer.vo += timer.pcpuTime();

	LOGDONE(2, 5);

	if (verbose > 3) {
		LOG0(" Eligible variables:");
		for (uint32 i = 0; i < eligible.size(); ++i) {
			uint32 v = eligible[i];
			LOG1("  e[%d]->(v: %d, p: %d, n: %d, s: %d)", i, v, occs[v].ps, occs[v].ns, scores[v]);
		}
	}

	// extended LCVE
	LOGN2(2, " Electing variables in phase-%d..", phase);

	sp->stacktail = sp->tmpstack;
	uint32*& tail = sp->stacktail;
	LIT_ST* frozen = sp->frozen;
	const uint32 maxoccurs = opts.lcve_max_occurs;
	const uint32 pmax = opts.mu_pos << multiplier;
	const uint32 nmax = opts.mu_neg << multiplier;
	const State_t* states = sp->state;
	const uint32* end = eligible.end();
	elected.clear();
	while (vars != end) {
		const uint32 cand = *vars++;
		CHECKVAR(cand);
		if (iassumed(cand)) continue;
		if (states[cand].state) continue;
		if (frozen[cand]) continue;
		const uint32 p = V2L(cand), n = NEG(p);
		assert((uint32)ot[p].size() >= occs[cand].ps);
		assert((uint32)ot[n].size() >= occs[cand].ns);
		if (!occs[cand].ps && !occs[cand].ns) continue;
		if (occs[cand].ps > maxoccurs || occs[cand].ns > maxoccurs) break;
		OL& poss = ot[p], &negs = ot[n];
		const uint32 ps = (uint32)poss.size(), ns = (uint32)negs.size();
		if (ps >= pmax && ns >= nmax) break;
		if (depFreeze(poss, occs, frozen, tail, cand, pmax, nmax) &&
			depFreeze(negs, occs, frozen, tail, cand, pmax, nmax))
			elected.push(cand);
	}

	assert(_verifyelected(elected, sp->frozen));

	if (opts.ve_fun_en) {
		clearMapFrozen();
		int coresize = 0;
		LIT_ST* marks = sp->marks;
		uint32* first = sp->tmpstack;
		assert(tail - first <= inf.maxVar);
		while (first != tail) {
			const uint32 v = *first++;
			CHECKVAR(v);
			if (coresize < MAXFUNVAR) {
				assert(UNASSIGNED(marks[v]));
				const LIT_ST mcore = coresize++;
				marks[v] = mcore;
				assert(marks[v] >= 0 && marks[v] < MAXFUNVAR);
				orgcore[mcore] = v;
			}
		}
	}

	clearFrozen();

	LOGENDING(2, 5, "(%d elected)", elected.size());

	if (verbose > 3) { 
		LOGN0(" PLCVs ");
		printVars(elected, elected.size(), 'v'); 
	}

	if (elected.size() < opts.lcve_min_vars) {
		if (verbose > 1) LOGWARN("elected variables not enough");
		return false;
	}

	return true;
}

inline bool Solver::depFreeze(OL& ol, OCCUR* occs, LIT_ST* frozen, uint32*& tail, const uint32& cand, const uint32& pmax, const uint32& nmax)
{
	const int maxcsize = opts.lcve_clause_max;
	uint32* first = tail;
	forall_occurs(ol, i) {
		SCLAUSE& c = scnf[*i];
		if (c.deleted()) continue;
		if (c.size() > maxcsize) {
			uint32 *from = first;
			while (from != tail) 
				frozen[*from++] = 0;
			tail = first;
			return false;
		}
		forall_clause(c, k) {
			const uint32 v = ABS(*k);
			CHECKVAR(v);
			if (!frozen[v] && NEQUAL(v, cand) && (occs[v].ps < pmax || occs[v].ns < nmax)) {
				frozen[v] = 1;
				assert(tail < sp->tmpstack + inf.maxVar);
				*tail++ = v;
			}
		}
	}
	return true;
}