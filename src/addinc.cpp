/***********************************************************************[addinc.cpp]
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

inline void printOriginal(Lits_t& clause)
{
	LOGN0("  adding original clause( ");
	forall_clause(clause, k) {
		PRINT("%d  ", SIGN(*k) ? -int(ABS(*k)) : int(ABS(*k)));
	}
	PRINT(")\n");
}

uint32 Solver::iadd() 
{
	inf.unassigned++;
	const uint32 v = inf.orgVars = ++inf.maxVar;
	LOG2(3, "  adding new variable %d (%d unassigned)..", v, inf.unassigned);
	const uint32 lit = V2L(v);
	inf.nDualVars = lit + 2;
	wt.expand(lit + 2);
	ivalue.expand(lit + 2, UNDEF_VAL);
	bumps.expand(v + 1, 0);
	chb.scores.expand(v + 1, 0);
	vsids.scores.expand(v + 1, 0);
	ilevel.expand(v + 1, UNDEF_LEVEL);
	ifrozen.expand(v + 1, 0);
	ivstate.expand(v + 1), ivstate[v] = State_t();
	model.maxVar = v;
	model.lits.expand(v + 1), model.lits[v] = lit;
	vorg.expand(v + 1), vorg[v] = v;
	vmtf.insert(v);
	vmtf.update(v, (bumps[v] = ++bumped));
	vsidsheap.insert(v);
	chbheap.insert(v);
	if (sp == NULL) { 
		sp = new SP();
		if (opts.proof_en)
			proof.init(sp);
	}
	sp->value = ivalue;
	sp->level = ilevel;
	sp->state = ivstate;
	return v;
}

#if defined(__linux__) || defined(__CYGWIN__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

bool Solver::itoClause(Lits_t& c, Lits_t& org)
{
	if (org.empty()) {
		learnEmpty();
		LOG2(2, "  original clause is empty.");
		return false;
	}
	assert(c.empty());
	assert(_verifymarkings(imarks, org));
	bool satisfied = false;
	if (verbose >= 3) printOriginal(org);
	LOGN2(3, "  adding mapped clause  ( ");
	forall_clause(org, k) {
		const uint32 orglit = *k;
		assert(orglit > 1 && orglit < UNDEF_VAR);
		const uint32 orgvar = ABS(orglit);
		const LIT_ST sign = SIGN(orglit);
		imarks.expand(orgvar + 1, UNDEF_VAL);
		LIT_ST marker = imarks[orgvar];
		if (UNASSIGNED(marker)) {
			assert(sign >= 0);
			imarks[orgvar] = sign;
			uint32 mlit = imap(orgvar);
			CHECKLIT(mlit);
			uint32 mvar = ABS(mlit);
			mlit = V2DEC(mvar, sign);
			PRINT2(3, 5, "%d  ", SIGN(mlit) ? -int(mvar) : int(mvar));
			LIT_ST val = sp->value[mlit];
			if (UNASSIGNED(val))
				c.push(mlit);
			else if (val) 
				satisfied = true; // satisfied unit
		}
		else if (NEQUAL(marker, sign)) 
			satisfied = true; // tautology
	}
	PRINT2(3, 5, ")\n");
	forall_clause(org, k) {
		const uint32 orglit = *k;
		assert(orglit > 1 && orglit < UNDEF_VAR);
		imarks[ABS(orglit)] = UNDEF_VAL;
	}
	if (satisfied) {
		if (opts.proof_en) proof.deleteClause(org);
	}
	else {
		int newsize = c.size();
		if (!newsize) {
			learnEmpty();
			LOG2(2, "  original clause became empty after parsing.");
			return false;
		}
		else if (newsize == 1) {
			const uint32 unit = *c;
			CHECKLIT(unit);
			LIT_ST val = sp->value[unit];
			if (UNASSIGNED(val)) enqueueUnit(unit), formula.units++;
			else if (!val) {
				LOG2(2, "  unit clause(%d) is conflicting.", l2i(unit));
				return false;
			}
		}
		else if (newsize) {
			if (newsize == 2) formula.binaries++;
			else if (newsize == 3) formula.ternaries++;
			else assert(newsize > 3), formula.large++;
			if (newsize > formula.maxClauseSize)
				formula.maxClauseSize = newsize;
			const C_REF newref = addClause(c, false);
			LOGCLAUSE(3, cm[newref], "  adding new clause");
		}
		if (opts.proof_en && newsize < org.size()) {
			proof.addClause(c);
			proof.deleteClause(org);
			org.clear();
		}
	}
	c.clear(), org.clear();
	return true;
}

#if defined(__linux__) || defined(__CYGWIN__)
#pragma GCC diagnostic pop
#endif