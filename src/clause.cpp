/***********************************************************************[clause.cpp]
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
#include "dimacs.hpp"

using namespace SeqFROST;

void Solver::removeClause(CLAUSE& c, const C_REF& cref)
{
	assert(cm[cref] == c);
	assert(!cm.deleted(cref));
	assert(c.size() > 1);
	const int size = c.size();
	if (c.learnt()) {
		assert(stats.clauses.learnt > 0);
		stats.clauses.learnt--;
		assert(stats.literals.learnt > 0);
		stats.literals.learnt -= size;
	}
	else {
		assert(c.original());
		assert(stats.clauses.original > 0);
		stats.clauses.original--;
		assert(stats.literals.original > 0);
		stats.literals.original -= size;
		stats.shrunken += size;
	}
	if (opts.proof_en) proof.deleteClause(c);
	cm.collectClause(cref, size);
	assert(cm.deleted(cref));
}

void Solver::addClause(const C_REF& cref, CLAUSE& c, const bool& learnt)
{
	assert(cm[cref] == c);
	const int size = c.size();
	assert(size > 1);
	if (learnt) {
		assert(sp->learntLBD > 0);
		int trimlbd = sp->learntLBD > size ? size : sp->learntLBD;
		c.markLearnt();
		c.set_lbd(trimlbd);
		c.set_usage(1 + (sp->learntLBD <= opts.lbd_tier2));
		if (size > 2 && trimlbd > opts.lbd_tier1) c.set_keep(0);
		learnts.push(cref);
		stats.clauses.learnt++;
		stats.literals.learnt += size;
	}
	else {
		assert(c.original());
		orgs.push(cref);
		stats.clauses.original++;
		stats.literals.original += size;
	}
	if (keeping(c)) 
		mark_subsume(c);
}

C_REF Solver::addClause(const Lits_t& src, const bool& learnt)
{
	C_REF r = UNDEF_REF;
	CLAUSE& c = *cm.alloc(r, src);
	assert(r != UNDEF_REF);
	assert(c.keep());
	assert(!cm.deleted(r));
	ATTACH_TWO_WATCHES(r, c);
	addClause(r, c, learnt);
	return r;
}

void Solver::newHyper2()
{
	assert(learntC.size() == 2);
	stats.binary.resolvents++;
	C_REF r = UNDEF_REF;
	CLAUSE& c = *cm.alloc(r, learntC);
	const uint32 first = c[0], second = c[1];
	DELAY_WATCH(first, second, r, 2);
	DELAY_WATCH(second, first, r, 2);
	sp->learntLBD = 2;
	addClause(r, c, true);
	c.markHyper();
	learntC.clear();
}

void Solver::newHyper3(const bool& learnt)
{
	const int size = learntC.size();
	assert(size > 1 && size <= 3);
	last.ternary.resolvents++;
	C_REF r = UNDEF_REF;
	CLAUSE& c = *cm.alloc(r, learntC);
	sp->learntLBD = size;
	addClause(r, c, learnt);
	if (learnt) c.markHyper();
	ATTACH_CLAUSE(r, c);
	LOGCLAUSE(4, c, "  added new hyper ternary resolvent");
}

inline LIT_ST Solver::sortClause(CLAUSE& c, const int& start, const int& size, const bool& satonly)
{
	assert(size > 1);
	assert(start < size);
	const LIT_ST* values = sp->value;
	uint32* lits = c.data();
	uint32 x = lits[start];
	CHECKLIT(x);
	LIT_ST xval = values[x];
	if (UNASSIGNED(xval) || (xval && satonly)) return xval;
	uint32 best = x;
	uint32 xlevel = l2dl(x);
	int pos = 0;
	CHECKLEVEL(xlevel);
	for (int i = start + 1; i < size; ++i) {
		const uint32 y = lits[i];
		CHECKLIT(y);
		const LIT_ST yval = values[y];
		if (UNASSIGNED(yval) || (yval && satonly)) {
			best = y;
			pos = i;
			xval = yval;
			break;
		}
		const uint32 ylevel = l2dl(y);
		CHECKLEVEL(ylevel);
		bool better;
		if (!xval && yval > 0) better = true;
		else if (xval > 0 && !yval) better = false;
		else if (!xval) {
			assert(!yval);
			better = (xlevel < ylevel);
		}
		else {
			assert(xval > 0);
			assert(yval > 0);
			assert(!satonly);
			better = (xlevel > ylevel);
		}
		if (better) {
			best = y;
			pos = i;
			xval = yval;
			xlevel = ylevel;
		}
	}
	if (!pos) return xval;
	lits[start] = best;
	lits[pos] = x;
	return xval;
}

void Solver::sortClause(CLAUSE& c)
{
	const int size = c.size();
	const LIT_ST val = sortClause(c, 0, size, false);
	if (size > 2) sortClause(c, 1, size, val);
}