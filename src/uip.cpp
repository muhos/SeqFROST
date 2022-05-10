/***********************************************************************[uip.cpp]
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

inline int Solver::calcLBD(CLAUSE& c) 
{
	const uint32* lits = c.data();
	int lbd = 0;
	if (c.binary()) 
		lbd = (l2dl(lits[0]) != l2dl(lits[1])) + 1;
	else {
		uint32* board = sp->board;
		const uint32 marker = ++stats.marker;
		for (const uint32* k = lits, *cend = c.end(); k != cend; ++k) {
			const uint32 litLevel = l2dl(*k);
			if (NEQUAL(board[litLevel], marker)) { 
				board[litLevel] = marker; 
				lbd++; 
			}
		}
	}
	return lbd;
}

inline void	Solver::bumpClause(CLAUSE& c)
{
	assert(c.learnt());
	assert(c.size() > 2);
	const bool hyper = c.hyper();
	if (!hyper && c.keep()) return;
	const CL_ST used = c.usage();
	c.initTier3();
	if (hyper) return;
	const int old_lbd = c.lbd();
	const int new_lbd = calcLBD(c);
	if (new_lbd < old_lbd) { // update old LBD
		if (new_lbd <= opts.lbd_tier1) c.set_keep(true);
		else if (old_lbd > opts.lbd_tier2 && new_lbd <= opts.lbd_tier2) c.initTier2();
		c.set_lbd(new_lbd);
		LOGCLAUSE(4, c, "  bumping clause with lbd %d ", new_lbd);
	}
	else if (used && old_lbd <= opts.lbd_tier2) c.initTier2();
}

inline void	Solver::analyzeLit(const uint32& lit, int& track, int& size)
{
	CHECKLIT(lit);
	assert(isFalse(lit));
	const uint32 v = ABS(lit);
	const uint32 litlevel = sp->level[v];
	CHECKLEVEL(litlevel);
	if (litlevel) {
		size++;
		if (sp->seen[v]) 
			return;
		sp->resolventsize++;
		sp->seen[v] = ANALYZED_M;
		analyzed.push(v);
		assert(litlevel <= LEVEL);
		if (litlevel == LEVEL) 
			track++;
		else if (litlevel < LEVEL) {
			sp->seen[v] = REMOVABLE_M;
			learntC.push(lit);
			State_t& state = sp->state[litlevel];
			if (state.dlcount & MAX_DLC)
				return;
			if (state.dlcount) {
				state.dlcount = MAX_DLC;
				return;
			}
			state.dlcount = 1;
			lbdlevels.push(litlevel);
		}
	}
}

inline bool Solver::analyzeReason(const C_REF& ref, const uint32& parent, int& track) 
{
	CHECKLIT(parent);
	CLAUSE& reason = cm[ref];
	LOGCLAUSE(4, reason, "  analyzing %d reason", l2i(parent));
	sp->reasonsize = 1;
	sp->conflictdepth++;
	if (reason.binary()) 
		analyzeLit((reason[0] ^ reason[1] ^ parent), track, sp->reasonsize);
	else {
		if (reason.learnt()) bumpClause(reason);
		forall_clause(reason, k) {
			const uint32 lit = *k;
			if (NEQUAL(lit, parent))
				analyzeLit(lit, track, sp->reasonsize);
		}
	}
	assert(sp->resolventsize > 0);
	sp->resolventsize--;
	if (sp->reasonsize > 2 && sp->resolventsize < sp->reasonsize) {
		assert(sp->resolventsize >= 0);
		assert(!reason.binary());
		assert(!cm.deleted(ref));
		strengthenOTF(reason, ref, parent);
		if (sp->conflictdepth == 1 && sp->resolventsize < sp->conflictsize) {
			assert(sp->conflictsize > 2);
			assert(conflict != UNDEF_REF);
			assert(ref != conflict);
			CLAUSE& subsumed = cm[conflict];
			assert(reason.size() <= subsumed.size());
			if (reason.original() || subsumed.learnt()) {
				LOGCLAUSE(4, subsumed, "  found subsumed conflict");
				removeClause(subsumed, conflict);
#ifdef STATISTICS
				stats.forward.subsumedfly++;
#endif
			}
		}
		conflict = ref;
		return true;
	}
	return false;
}

inline void Solver::strengthenOTF(CLAUSE& c, const C_REF& ref, const uint32& self)
{
	CHECKLIT(self);
	assert(c.size() > 2);
	assert(c[0] == self || c[1] == self);

#ifdef LOGGING
	LOG2(4, "  parent(%d) is strengthening last reason clause", l2i(self));
#endif
#ifdef STATISTICS
	stats.forward.strengthenedfly++;
#endif

	uint32* lits = c.data();

	const uint32 other = lits[0] ^ lits[1] ^ self;
	lits[0] = other;
	lits[1] = self;
	assert(lits[0] != lits[1]);

	detachWatch(FLIP(self), ref);

	if (opts.proof_en)
		proof.shrinkClause(c, self);

	uint32* j = lits + 1, *end = c.end();
	for (uint32* i = lits + 2; i != end; ++i) {
		const uint32 lit = *i;
		assert(isFalse(lit));
		if (l2dl(lit))
			*j++ = *i;
	}

	const int removed = int(end - j);

	shrinkClause(c, removed);

	const int size = c.size();
	int maxPos = 1;
	uint32 maxLevel = l2dl(lits[1]);

	for (int i = 2; i < size; ++i) {
		const uint32 other = lits[i];
		const uint32 otherLevel = l2dl(other);
		if (otherLevel > maxLevel) {
			maxPos = i;
			maxLevel = otherLevel;
		}
	}

	if (NEQUAL(maxPos, 1)) 
		std::swap(lits[1], lits[maxPos]);

	const uint32 first = lits[0];
	const uint32 second = lits[1];
	ATTACH_WATCH(second, first, ref, size);

	WL& ws = wt[FLIP(first)];
	forall_watches(ws, i) {
		if (i->ref == ref) {
			i->imp = second;
			i->size = size;
			break;
		}
	}
}

bool Solver::finduip()
{
	assert(UNSOLVED);
	assert(conflict != UNDEF_REF);
	CHECKLEVEL(LEVEL);

	sp->learntLBD = UNDEFINED;
	sp->reasonsize = 0;
	sp->conflictsize = 0;
	sp->resolventsize = 0;
	sp->conflictdepth = 0;

	learntC.push(0);

	int track = 0;

	CLAUSE& c = cm[conflict];

	LOGCLAUSE(4, c, "  analyzing conflict");

	uint32* lits = c.data();
	if (c.binary()) {
		analyzeLit(lits[0], track, sp->conflictsize);
		analyzeLit(lits[1], track, sp->conflictsize);
	}
	else {

		if (c.learnt()) 
			bumpClause(c);

		for (uint32* k = lits, *cend = c.end(); k != cend; ++k) {
			analyzeLit(*k, track, sp->conflictsize);
		}
	}
	// analyze reasons
	assert(sp->conflictsize == sp->resolventsize);
	assert(!sp->reasonsize);
	assert(!sp->conflictdepth);
	uint32 parent = 0;
	uint32* tail = trail.end();
	while (true) {
		while (!parent) { // find next implication 
			assert(tail > trail.data());
			const uint32 lit = *--tail, v = ABS(lit);
			if (sp->seen[v] && sp->level[v] == LEVEL) 
				parent = lit;
		}
		assert(track);
		if (!--track) break;
		CHECKLIT(parent);
		const C_REF reason = l2r(parent);
		assert(REASON(reason));
		if (analyzeReason(reason, parent, track)) {
			learntC.clear();
			clearLevels();
			clearAnalyzed();
			return true;
		}
		parent = 0;
	}
	assert(learntC[0] == 0);
	learntC[0] = FLIP(parent);
	LOGLEARNT(this, 3);
	sp->learntLBD = (int)lbdlevels.size();
	assert(sp->learntLBD >= 0);
	assert(sp->learntLBD <= learntC.size());
	LOG2(4, "  LBD of learnt clause = %d", sp->learntLBD);
	return false;
}