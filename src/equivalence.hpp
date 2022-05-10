/***********************************************************************[equivalence.hpp]
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

#ifndef __EQUIVALENCE_
#define __EQUIVALENCE_

#include "simplify.hpp" 
#include "sort.hpp"

using namespace SeqFROST;

inline uint32 substitute_single(const uint32& dx, SCLAUSE& org, const uint32& def)
{
	CHECKLIT(dx);
	assert(def != dx);
	assert(org.original());

	LOGCLAUSE(4, org, " Clause ");

	uint32* j = org;
	forall_clause(org, i) {
		const uint32 lit = *i;
		CHECKLIT(lit);
		if (lit == dx) 
			*j++ = def;
		else if (NEQUAL(lit, def)) 
			*j++ = lit;
	}
	org.resize(int(j - org));

	if (org.size() == 1) 
		return *org;

	SORT(org);

	org.calcSig();

	LOGCLAUSE(4, org, " Substituted to ");

	return 0;
}

inline bool substitute_single(const uint32& p, const uint32& def, SCNF& scnf, OT& ot)
{
	CHECKLIT(def);
	assert(!SIGN(p));
	const uint32 n = NEG(p), def_f = FLIP(def);
	OL& poss = ot[p], & negs = ot[n];
	const bool proofEN = solver->opts.proof_en;
	// substitute negatives 
	forall_occurs(negs, i) {
		SCLAUSE& neg = scnf[*i];
		if (neg.learnt() || neg.molten() || neg.has(def))
			neg.markDeleted();
		else if (neg.original()) {
			uint32 unit = substitute_single(n, neg, def_f);
			if (unit) {
				const LIT_ST val = solver->l2value(unit);
				if (UNASSIGNED(val))
					solver->enqueueUnit(unit);
				else if (!val) return true;
			}
			else if (proofEN)
				solver->proof.addResolvent(neg);
		}
	}
	// substitute positives
	forall_occurs(poss, i){
		SCLAUSE& pos = scnf[*i];
		if (pos.learnt() || pos.molten() || pos.has(def_f))
			pos.markDeleted();
		else if (pos.original()) {
			uint32 unit = substitute_single(p, pos, def);
			if (unit) {
				const LIT_ST val = solver->l2value(unit);
				if (UNASSIGNED(val))
					solver->enqueueUnit(unit);
				else if (!val) return true;
			}
			else if (proofEN)
				solver->proof.addResolvent(pos);
		}
	}
	return false; 
}

inline uint32 find_sfanin(const uint32& gate_out, SCNF& scnf, OL& list)
{
	CHECKLIT(gate_out);
	uint32 imp = 0;
	int nImps = 0;
	forall_occurs(list, i) {
		SCLAUSE& c = scnf[*i];
		if (c.original() && c.size() == 2) {
			imp = FLIP(c[0] ^ c[1] ^ gate_out);
			c.melt(); // mark as gate clause
			nImps++;
		}
		if (nImps > 1) {
			// cannot be a single-input gate	
			return 0;
		}
	}
	return imp;
}

inline uint32 find_BN_gate(const uint32& p, const uint32& n, SCNF& scnf, OL& poss, OL& negs)
{
	assert(n == NEG(p));
	assert(poss.size());
	assert(negs.size());

	if (scnf[*poss].size() > 2 ||
		scnf[*negs].size() > 2) {
		return false;
	}

	assert(checkMolten(scnf, poss, negs));

	uint32 first = find_sfanin(p, scnf, poss);

	if (first) {
		uint32 second = n, def = first;
		if (second < first) {
			first = second;
			second = def;
		}
		forall_occurs(negs, i) {
			SCLAUSE& c = scnf[*i];
			if (c.original() && c.size() == 2 && c[0] == first && c[1] == second) {

				c.melt(); // mark as fanout clause

				LOG2(3, " Gate %d = -/+%d found", ABS(p), ABS(def));
				LOGOCCURS(solver, 4, ABS(p));

				return def;
			}
		}
	}

	freezeBinaries(scnf, poss);

	return 0;
}

inline void save_BN_gate(const uint32& p, const int& pOrgs, const int& nOrgs, SCNF& scnf, OL& poss, OL& negs, MODEL& model)
{
	CHECKLIT(p);
	LOG2(4, " saving buffer/inverter clauses as witness");
	const uint32 n = NEG(p);
	if (pOrgs > nOrgs) {
		forall_occurs(negs, i) {
			SCLAUSE& c = scnf[*i];
			if (c.original())
				model.saveClause(c, c.size(), n);
		}
		model.saveWitness(p);
	}
	else {
		forall_occurs(poss, i) {
			SCLAUSE& c = scnf[*i];
			if (c.original())
				model.saveClause(c, c.size(), p);
		}
		model.saveWitness(n);
	}
}

#endif