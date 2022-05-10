/***********************************************************************[reduce.cpp]
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

#include "can.hpp"
#include "sort.hpp"
#include "solve.hpp"

using namespace SeqFROST;

#if defined(STDSORTSTB)

struct REDUCED_CMP {
	const CMM& cm;
	REDUCED_CMP(const CMM& _cm) : cm(_cm) {}
	bool operator () (const C_REF& a, const C_REF& b) const {
		const CLAUSE& x = cm[a], & y = cm[b];
		const int xl = x.lbd(), yl = y.lbd();
		if (xl > yl) return true;
		if (xl < yl) return false;
		return x.size() > y.size();
	}
};

#else 

struct REDUCED_CMP {
	const CMM& cm;
	REDUCED_CMP(const CMM& _cm) : cm(_cm) {}
	inline int operator () (const C_REF* a, const C_REF* b) const {
		const CLAUSE& x = cm[*a], & y = cm[*b];
		int xl = x.lbd(), yl = y.lbd();
		if (xl > yl) return true;
		if (xl < yl) return false;
		return x.size() >= y.size();
	}
};

#endif

bool Solver::chronoHasRoot()
{
	if (!opts.chrono_en || !LEVEL) 
		return true;
	for (uint32* i = trail + dlevel[1].depth, *end = trail.end(); i != end; ++i) {
		assert(!unassigned(*i));
		if (!l2dl(*i)) {
			LOG2(2, " Found root unit(%d) due to chronological backtracking", l2i(*i));
			backtrack();
			if (BCP()) { 
				learnEmpty();
				return false;
			}
			return true;
		}
	}
	return true;	
}

void Solver::reduce()
{
	assert(sp->propagated == trail.size());
	assert(conflict == UNDEF_REF);
	assert(UNSOLVED);
	assert(learnts.size());
	stats.reduces++;
	if (!chronoHasRoot()) 
		return;
	if (canForward()) 
		forward();
	const bool shrunken = shrink();
	if (learnts.empty()) 
		return;
	markReasons();
	reduceLearnts();
	recycle();
	unmarkReasons();
	INCREASE_LIMIT(reduce, stats.reduces, nbylogn, false);
	if (shrunken && canMap()) 
		map(); // "recycle" must be called beforehand
}

void Solver::reduceLearnts()
{
	assert(reduced.empty());
	assert(learnts.size());
	reduced.reserve(learnts.size());
	PREFETCH_CM(cs, deleted);
	C_REF* end = learnts.end();
	for (C_REF* i = learnts; i != end; ++i) {
		const C_REF r = *i;
		if (deleted[r]) continue;
		assert(!cm.deleted(r));
		GET_CLAUSE(c, r, cs);
		assert(c.learnt());
		if (c.reason()) continue;
		if (c.hyper()) {
			assert(c.size() <= 3);
			if (c.usage()) c.warm();
			else {
				removeClause(c, r);
#ifdef STATISTICS
				if (c.binary()) stats.binary.reduced++;
				else stats.ternary.reduced++;
#endif
			}
			continue;
		}
		if (c.keep()) continue;
		if (c.usage()) {
			c.warm();
			if (c.lbd() <= opts.lbd_tier2) continue;
		}
		assert(c.size() > 2);
		reduced.push(r);
	}
	const C_REF rsize = reduced.size();
	if (rsize) {
		C_REF pivot = opts.reduce_perc * rsize;
		LOGN2(2, " Reducing learnt database up to (%zd clauses)..", pivot);
		end = reduced.end();
		C_REF* head = reduced.data();
		STABLESORT(head, end, rsize, REDUCED_CMP(cm));
		C_REF *tail = reduced + pivot;
		for (; head != tail; ++head) {
			const C_REF r = *head;
			GET_CLAUSE(c, r, cs);
			assert(c.learnt());
			assert(!c.reason());
			assert(!c.keep());
			assert(c.lbd() > opts.lbd_tier1);
			assert(c.size() > 2);
			removeClause(c, r);
		}
		limit.keptsize = 0, limit.keptlbd = 0;
		for (head = tail; head != end; ++head) {
			GET_CLAUSE(c, *head, cs);
			if (c.lbd() > limit.keptlbd) limit.keptlbd = c.lbd();
			if (c.size() > limit.keptsize) limit.keptsize = c.size();
		}
		reduced.clear();
		LOGENDING(2, 5, "(kept lbd: %d, size: %d)", limit.keptlbd, limit.keptsize);
	}
}