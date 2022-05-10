/***********************************************************************[minimizeall.cpp]
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
#include "sort.hpp"

using namespace SeqFROST;

inline int Solver::analyzeLit(const uint32& level, const uint32& lit)
{
	CHECKLIT(lit);
	assert(!sp->value[lit]);
	const uint32 v = ABS(lit);
	const uint32 litlevel = sp->level[v];
	assert(litlevel <= level);
	LIT_ST* seen = sp->seen;
	if (!litlevel || SHRINKABLE(seen[v])) return 0;
	if (litlevel < level) {
		if (REMOVABLE(seen[v]) || minimize(FLIP(lit), 1)) return 0;
		return -1;
	}
	seen[v] = SHRINKABLE_M;
	shrinkable.push(v);
	return 1;
}

inline void Solver::minimizeBlock(LEARNTLIT* bbegin, const LEARNTLIT* bend, const uint32& level, const uint32& uip)
{
	CHECKLIT(uip);
	assert(bbegin < bend);
	LOG2(2, "  found UIP %d at level %d", l2i(uip), level);
	const uint32 uipvar = ABS(uip);
	const uint32 fip = FLIP(uip);
	const uint32 uiplevel = sp->level[uipvar];
	sp->state[uiplevel].dlcount = 1;
	// for bumping later
	if (!ANALYZED(sp->seen[uipvar])) {
		sp->seen[uipvar] = ANALYZED_M; 
		analyzed.push(uipvar);
	}
	// replace the entire block with deduced UIP
	uint32 removed = 0;
	for (LEARNTLIT* k = bbegin; k != bend; ++k) {
		if (k->lit) {
			k->lit = 0;
			learntC[k->idx] = 0;
			removed++;
		}
	}
	bbegin->lit = fip;
	learntC[bbegin->idx] = fip;

	while (shrinkable.size()) {
		const uint32 v = shrinkable.back();
		shrinkable.pop();
		assert(SHRINKABLE(sp->seen[v]) || ANALYZED(sp->seen[v]));
		sp->seen[v] = REMOVABLE_M;
		minimized.push(v);
	}
	assert(removed > 0);
	removed--;
	stats.alluip.shrunken += removed;
	LOG2(2, " replaced %d literals at level %d with UIP %d", removed, level, l2i(fip));
}

inline uint32 Solver::analyzeReason(bool& failed, const uint32& level, const uint32& uip)
{
	const uint32 uipvar = ABS (uip);
	assert(SHRINKABLE(sp->seen[uipvar]));
	assert(sp->level[uipvar] == level);
	const C_REF ref = sp->source[uipvar];
	assert(REASON(ref));
	CLAUSE& c = cm[ref];
	LOGCLAUSE(4, c, "  checking %d reason", l2i(uip));
	stats.searchticks++;
	uint32 bsize = 0;
	forall_clause(c, k) {
		const uint32 other = *k;
		if (other == uip) continue;
		assert(!sp->value[other]);
		int result = analyzeLit(level, other);
		if (result < 0) {
			failed = true;
			break;
		}
		if (result > 0)
			bsize++;
	}
	return bsize;
}

inline void Solver::analyzeBlock(LEARNTLIT* bbegin, const LEARNTLIT* bend, const uint32& level, const uint32& maxtrail)
{
	assert(level < LEVEL);
	assert(bbegin < bend);
	uint32 bsize = uint32(bend - bbegin);
	assert(bsize > 1);
	for (const LEARNTLIT* k = bbegin; k != bend; ++k) {
		const uint32 lit = k->lit;
		if (lit) {
			CHECKLIT(lit);
			analyzeLit(level, lit);
		}
	}
	assert(shrinkable.size() == bsize);
	uint32 uip = 0;
	bool failed = false;
	const uint32* tail = trail + maxtrail;
	while (!failed) {
		do {
			assert(trail <= tail);
			uip = *tail--;
		}
		while (!SHRINKABLE(sp->seen[ABS(uip)]));
		if (bsize == 1) break;
		bsize += analyzeReason(failed, level, uip);
		assert(bsize > 1);
		bsize--;
	}
	if (failed) {
		while (shrinkable.size()) {
			const uint32 v = shrinkable.back();
			shrinkable.pop();
			assert(SHRINKABLE(sp->seen[v]));
			sp->seen[v] = 0;
		}
	}
	else
		minimizeBlock(bbegin, bend, level, uip);
	assert(shrinkable.empty());
}

void Solver::minimizeall() 
{
	LOG2(4, " Starting learnt clause minimization by all UIPs");
	uint64 before = stats.alluip.shrunken;
	Vec<LEARNTLIT> learntCopy;
	const int learntsize = learntC.size();
	learntCopy.resize(learntsize);
	for (int i = 0; i < learntsize; ++i) {
		learntCopy[i] = LEARNTLIT(learntC[i], i);
	}
	if (!opts.minimizesort_en)
		QSORTCMP(learntCopy, LEARNTLIT_CMP(sp));
	LEARNTLIT *copyend = learntCopy.end(), *bend = copyend;
	LEARNTLIT *copystart = learntCopy;
	while (bend != copystart) {
		uint32 level;
		uint32 maxtrail;
		LEARNTLIT* bbegin = nextLitBlock(sp, copystart, bend, level, maxtrail);
		uint32 bsize = uint32(bend - bbegin);
		assert(bsize > 0);
		if (bsize >= 2)
			analyzeBlock(bbegin, bend, level, maxtrail);
		bend = bbegin;
	}
	uint32* j = learntC, *learntend = learntC.end();
	for (const uint32* i = j; i != learntend; ++i) {
		const uint32 lit = *i;
		if (lit) 
			*j++ = lit;
	}
	assert(stats.alluip.shrunken >= before);
	uint32 removed = stats.alluip.shrunken - before;
	assert(j + removed == learntC.end());
	learntC.shrink(removed);
	clearMinimized();
	LOG2(4, " Learnt clause minimized by %d literals", removed);
}