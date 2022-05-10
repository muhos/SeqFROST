/***********************************************************************[analyze.cpp]
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

inline void	Solver::bumpReason(const uint32& lit) 
{
	CHECKLIT(lit);
	assert(isFalse(lit));

	const uint32 v = ABS(lit);

	CHECKLEVEL(sp->level[v]);

	if (!sp->level[v] || ANALYZED(sp->seen[v])) return;

#ifdef LOGGING
	LOG2(4, "  bumping reason literal %d@%d", l2i(lit), sp->level[v]);
#endif

	assert(!sp->seen[v]);

	sp->seen[v] = ANALYZED_M;
	analyzed.push(v);
}

inline void	Solver::bumpReasons(const uint32& lit)
{
	CHECKLIT(lit);
	const uint32 v = ABS(lit);
	const C_REF r = sp->source[v];
	if (REASON(r)) {
		CLAUSE& c = cm[r];
		if (c.binary()) {
			const uint32 other = c[0] ^ c[1] ^ lit;
			bumpReason(other);
		}
		else {
			forall_clause(c, k) {
				const uint32 other = *k;
				if (NEQUAL(other, lit)) 
					bumpReason(other);
			}
		}
	}
}

inline void	Solver::bumpReasons()
{
	assert(!probed);
	forall_clause(learntC, k) {
		sp->seen[ABS(*k)] = ANALYZED_M;
	}
	forall_clause(learntC, k) {
		assert(l2dl(*k) > 0);
		bumpReasons(FLIP(*k));
	}
}

inline void	Solver::bumpAnalyzed() 
{
	if (opts.bumpreason_en) 
		bumpReasons();

	if (!stable) 
		QSORTCMP(analyzed, QUEUE_CMP(bumps));

	forall_vector(uint32, analyzed, a) {
		const uint32 v = *a;
		assert(!sp->state[v].state);
		if (stable) { 
			vsids.bump(vsidsheap, v);
		}
		else 
			varBumpQueue(v);
	}

	if (stable) 
		vsids.boost();
}

bool Solver::chronoAnalyze()
{
	assert(conflict != UNDEF_REF);
	uint32 conflictlevel = 0;
	uint32 count = 0;
	uint32 forced = 0;
	CLAUSE& c = cm[conflict];
	forall_clause(c, k) {
		const uint32 lit = *k;
		const uint32 litlevel = l2dl(lit);
		if (litlevel > conflictlevel) {
			conflictlevel = litlevel;
			forced = lit;
			count = 1;
		}
		else if (litlevel == conflictlevel) {
			count++;
			if (conflictlevel == LEVEL && count > 1)
				break;
		}
	}
	assert(count);
	LOG2(3, "  found %d literals on conflict level %d", count, conflictlevel);

	if (!conflictlevel) { 
		learnEmpty(); 
		return false;
	}

	const int size = c.size();
	uint32* lits = c.data();
	for (int i = 0; i < 2; ++i) {
		uint32 lit = lits[i];
		uint32 maxLit = lit;
		int maxPos = i;
		uint32 maxLevel = l2dl(maxLit);
		for (int j = i + 1; j < size; ++j) {
			const uint32 other = lits[j];
			uint32 otherLevel = l2dl(other);
			if (maxLevel >= otherLevel) continue;
			maxPos = j;
			maxLit = other;
			maxLevel = otherLevel;
			if (maxLevel == conflictlevel) 
				break;
		}
		if (maxPos == i) continue;

		if (maxPos > 1) {
			detachWatch(FLIP(lit), conflict);
		}

		lits[maxPos] = lit;
		lits[i] = maxLit;

		if (maxPos > 1) {
			ATTACH_WATCH(maxLit, lits[!i], conflict, size);
		}
	}

	if (count == 1) {
		assert(forced > 1);
		assert(conflictlevel > 0);
		backtrack(conflictlevel - 1);
		enqueue(forced, forcedLevel(forced, c), conflict);
		LOGCLAUSE(3, c, "  forced %d@%d in conflicting clause", l2i(forced), l2dl(forced));
		conflict = UNDEF_REF;
		return true;
	}

	backtrack(conflictlevel);

	return false;
}

void Solver::analyze()
{
	assert(conflict != UNDEF_REF);
	assert(learntC.empty());
	assert(analyzed.empty());
	assert(lbdlevels.empty());

#ifdef LOGGING
	LOG2(3, " Analyzing conflict%s:", probed ? " during probing" : "");
	LOGTRAIL(this, 4);
#endif

	bool conflictchanged = true;
	while (conflictchanged) {
		stats.conflicts++;
		if (opts.chrono_en && chronoAnalyze()) return;
		if (UNSAT) return;
		if (!LEVEL) { learnEmpty(); return; }
		// find first-UIP
		conflictchanged = finduip();
	}

	// check overflow
	if (stats.marker >= MAX_MARKER) {
		stats.marker = 0;
		stats.markerresets++;
		sp->clearBoard();
	}

	// update luby restart
	if (stable) lubyrest.update();

	if (!probed) {

		// minimize learnt clause 
#ifdef STATISTICS
		stats.minimize.before += learntC.size();
#endif

		if (opts.minimizesort_en)
			QSORTCMP(learntC, MINIMIZE_CMP(sp));
		if (opts.minimize_en && learntC.size() > 1)
			minimize();
		if (opts.minimizeall_en && learntC.size() > 1)
			minimizeall();
		if (opts.minimizebin_en && learntC.size() > 1
			&& sp->learntLBD <= opts.minimize_lbd
			&& learntC.size() <= opts.minimize_min)
			minimizebin();

#ifdef STATISTICS
		stats.minimize.after += learntC.size();
#endif

		// update lbd restart mechanism
		lbdrest.update(stable, sp->learntLBD);

		// bump variable activities
		if (!stable || decheuristic == 0)
			bumpAnalyzed();

		if (stable && decheuristic == 1)
			updateCHB();
	}
	else
		assert(learntC.size() == 1);

	// learn control
	C_REF added = learn();

	// clear 
	conflict = UNDEF_REF;
	learntC.clear();
	clearLevels();
	clearAnalyzed();

	// subsume recent learnts 
	if (opts.learntsub_max && REASON(added)) 
		subsumeLearnt(added);
}

void Solver::subsumeLearnt(const C_REF& lref)
{
	if (learnts.size() < 2) return;
	assert(lref != UNDEF_REF);
	PREFETCH_CM(cs, deleted)
	GET_CLAUSE(learnt, lref, cs);
	mark_literals(learnt);
	const int learntsize = learnt.size();
	uint64 trials = stats.subtried + opts.learntsub_max;
	C_REF* tail = learnts.end(), * head = learnts;

#ifdef STATISTICS
	uint32 nsubsumed = 0;
#endif

	while (tail != head && stats.subtried++ <= trials) {
		const C_REF t = *--tail;
		if (lref == t) continue;
		if (deleted[t]) continue;
		GET_CLAUSE(c, t, cs);
		int sub = learntsize;
		forall_clause(c, k) {
			if (subsumed(*k) && !--sub) {
#ifdef STATISTICS
				nsubsumed++;
#endif
				removeClause(c, t);
				break;
			}
		}
	}

	unmark_literals(learnt);

#ifdef STATISTICS
	stats.forward.learntfly += nsubsumed;
#endif
}