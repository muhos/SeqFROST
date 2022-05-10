/***********************************************************************[shrink.cpp]
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

inline void Solver::bumpShrunken(CLAUSE& c)
{
	assert(c.learnt());
	assert(c.size() > 1);
	if (c.keep()) return;
	if (c.hyper()) return;
	const int old_lbd = c.lbd();
	const int size = c.size() - 1;
	const int new_lbd = MIN(size, old_lbd);
	if (new_lbd >= old_lbd) return;
	if (new_lbd <= opts.lbd_tier1) c.set_keep(1);
	else if (old_lbd > opts.lbd_tier2 && new_lbd <= opts.lbd_tier2) c.initTier2();
	c.set_lbd(new_lbd);
	LOGCLAUSE(4, c, " Bumping shrunken clause with LBD %d ", new_lbd);
}

inline int Solver::removeRooted(CLAUSE& c, const uint32* levels)
{
	uint32* j = c;
	forall_clause(c, i) {
		const uint32 lit = *i;
		CHECKLIT(lit);
		if (levels[ABS(lit)]) *j++ = lit;
		else assert(!sp->value[lit]);
	}
	return int(c.end() - j);
}

void Solver::shrinkClause(CLAUSE& c, const int& remLits)
{
	assert(remLits >= 0);
	if (!remLits) return;
	c.shrink(remLits); // adjusts "pos" also
	assert(c.size() > 1);
	if (c.learnt()) {
		bumpShrunken(c); // all shrunken must get bumped to "update" new binaries
		assert(stats.literals.learnt > 0);
		stats.literals.learnt -= remLits;
	}
	else {
		assert(c.original());
		assert(stats.literals.original > 0);
		stats.literals.original -= remLits;
	}
	if (keeping(c)) 
		mark_subsume(c);
	cm.collectLiterals(remLits);
}

void Solver::shrinkClause(const C_REF& r, const uint32* levels, const LIT_ST* values)
{
	CLAUSE& c = cm[r];
	assert(!cm.deleted(r));
	assert(c.size() > 2);
	uint32* i, * cend = c.end();
	int numNonFalse = 0;
	for (i = c; numNonFalse < 2 && i != cend; ++i) {
		const uint32 lit = *i;
		CHECKLIT(lit);
		if (values[lit]) {
			assert(l2dl(lit));
			numNonFalse++;
		}
	}
	if (numNonFalse < 2) return;
	if (opts.proof_en) proof.shrinkClause(c);
	shrinkClause(c, removeRooted(c, levels));
}

bool Solver::shrink()
{
	if (sp->simplified >= inf.maxFrozen) return false;
	sp->simplified = inf.maxFrozen;
	LOGN2(2, " Shrinking all clauses..");
	assert(trail.size());
	assert(conflict == UNDEF_REF);
	assert(UNSOLVED);
	assert(sp->propagated == trail.size());
	assert(!unassigned(trail.back()));
#ifdef STATISTICS
	stats.shrink.calls++;
	uint64 beforeCls = MAXCLAUSES, beforeLits = MAXLITERALS;
#endif
	shrink(orgs);
	shrink(learnts);
	assert(orgs.size() == ORIGINALS);
	assert(learnts.size() == LEARNTS);
#ifdef STATISTICS
	LOGSHRINKALL(2, beforeCls, beforeLits);
#else 
	LOGDONE(2, 5);
#endif
	return true;
}

void Solver::shrinkTop(const bool& conditional)
{
	if (conditional && sp->simplified >= inf.maxFrozen) return;
	assert(UNSOLVED);
	assert(conflict == UNDEF_REF);
	assert(UNSOLVED);
	assert(sp->propagated == trail.size());
	LOGN2(2, " Shrinking all clauses on top level..");
	if (sp->simplified < inf.maxFrozen) sp->simplified = inf.maxFrozen;
#ifdef STATISTICS
	stats.shrink.calls++;
	uint64 beforeCls = MAXCLAUSES, beforeLits = MAXLITERALS;
#endif
	shrinkTop(orgs);
	shrinkTop(learnts);
	assert(orgs.size() == ORIGINALS);
	assert(learnts.size() == LEARNTS);
#ifdef STATISTICS
	LOGSHRINKALL(2, beforeCls, beforeLits);
#else 
	LOGDONE(2, 5);
#endif
}

void Solver::shrink(BCNF& cnf)
{
	if (cnf.empty()) return;
	const uint32* levels = sp->level;
	const LIT_ST* values = sp->value;
	PREFETCH_CM(cs, deleted);
	C_REF* j = cnf;
	forall_cnf(cnf, i) {
		const C_REF r = *i;
		if (deleted[r]) continue;
		GET_CLAUSE(c, r, cs);
		assert(!c.moved());
		CL_ST st = -1;
		forall_clause(c, k) {
			const uint32 lit = *k;
			CHECKLIT(lit);
			if (!levels[ABS(lit)]) {
				const LIT_ST val = values[lit];
				assert(!UNASSIGNED(val));
				if (val > 0) {
					removeClause(c, r);
					st = 1;
					break;
				}
				if (!val) 
					st = 0;
			}
		}
		if (!st) {
			shrinkClause(r, levels, values);
			*j++ = r;
		}
		else if (st < 0)
			*j++ = r;
	}
	assert(j >= cnf);
	cnf.resize(uint32(j - cnf));
}

void Solver::shrinkTop(BCNF& cnf)
{
	if (cnf.empty()) return;
	assert(!LEVEL);
	const uint32* levels = sp->level;
	const LIT_ST* values = sp->value;
	PREFETCH_CM(cs, deleted);
	C_REF* j = cnf;
	forall_cnf(cnf, i) {
		const C_REF r = *i;
		if (deleted[r]) continue;
		GET_CLAUSE(c, r, cs);
		assert(!c.moved());
		CL_ST st = -1;
		forall_clause(c, k) {
			const uint32 lit = *k;
			CHECKLIT(lit);
			const LIT_ST val = values[lit];
			if (val > 0) {
				removeClause(c, r);
				st = 1;
				break;
			}
			if (!val) 
				st = 0;
		}
		if (!st) {
			shrinkClause(c, removeRooted(c, levels));
			*j++ = r;
		}
		else if (st < 0) 
			*j++ = r;
	}
	assert(j >= cnf);
	cnf.resize(uint32(j - cnf));
}