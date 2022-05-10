/***********************************************************************[vivify.cpp]
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
#include "histogram.hpp"

using namespace SeqFROST;

// In clauses scheduling, binary clauses are also considered
// for histogram but ignored in the actual vivification
// this gives proper indication of what literals are more
// important to vivify

#define VIVIFYORG			1
#define VIVIFYTIER1			2
#define VIVIFYTIER2			4
#define ISVIVIFYORG(x)		((x) & VIVIFYORG)
#define ISVIVIFYTIER1(x)	((x) & VIVIFYTIER1)
#define ISVIVIFYTIER2(x)	((x) & VIVIFYTIER2)

struct VIVIFY_WORSE_CMP {
	CMM& cm;
	const HIST_MCV_CMP& clauseKey;
	VIVIFY_WORSE_CMP(CMM& _cm, const HIST_MCV_CMP& _ck) :
		cm(_cm), clauseKey(_ck) {}
	inline bool operator() (const C_REF& r1, const C_REF& r2) {
		CLAUSE& c = cm[r1];
		CLAUSE& d = cm[r2];
		if (!c.vivify() && d.vivify()) return true;
		if (c.vivify() && !d.vivify()) return false;
		uint32* i = c, *j = d;
		uint32* cend = c.end(), *dend = d.end();
		while (i != cend && j != dend) {
			const uint32 a = *i++, b = *j++;
			if (NEQUAL(a, b)) return clauseKey(b, a);
		}
		if (i != cend && j == dend) return true;
		if (i == cend && j != dend) return false;
		return r1 < r2;
	}
};

void Solver::sortVivify(BCNF& cnf)
{
	if (cnf.empty()) return;
	HIST_MCV_CMP clauseKey(vhist);
	forall_cnf(cnf, i) {
		QSORTCMP(cm[*i], clauseKey);
	}
	QSORTCMP(cnf, VIVIFY_WORSE_CMP(cm, clauseKey));
}

bool Solver::vivifyAnalyze(CLAUSE& cand, bool& original)
{
	assert(learntC.empty());
	assert(analyzed.empty());
	assert(conflict < UNDEF_REF);
	assert(LEVEL);
	CLAUSE& conf = cm[conflict];
	bool conflictoriginality = conf.original();
	uint32* levels = sp->level;
	C_REF* sources = sp->source;
	LIT_ST* values = sp->value, *seen = sp->seen;
	assert(conf.size() > 1);

	LOGCLAUSE(4, conf, "  analyzing conflict");

	forall_clause(conf, k) {
		const uint32 lit = *k, v = ABS(lit);
		CHECKVAR(v);
		assert(isFalse(lit));
		if (levels[v]) {
			assert(!seen[v]);
			seen[v] = ANALYZED_M;
			analyzed.push(lit);
		}
	}

	mark_literals(cand);

	bool candsubsumed = false;
	const bool candlearnt = cand.learnt();
	if (candlearnt || conflictoriginality) {
		candsubsumed = true;
		forall_clause(conf, k) {
			const uint32 lit = *k;
			if (levels[ABS(lit)] || UNASSIGNED(values[lit])) {
				if (notsubsumed(lit)) {
					candsubsumed = false;
					break;
				}
			}
		}
	}

	for (uint32 a = 0; !candsubsumed && a < analyzed.size(); ++a) {
		const uint32 flit = analyzed[a], lit = FLIP(flit);
		const uint32 v = ABS(lit);
		CHECKVAR(v);
		assert(levels[v]);
		assert(seen[v]);
		const C_REF src = sources[v];
		if (REASON(src)) {
			CLAUSE& reason = cm[src];
			LOGCLAUSE(4, reason, "  analyzing %d reason", l2i(lit));
			if (reason.learnt()) conflictoriginality = false;
			if (reason.binary()) {
				const uint32 other = reason[0] ^ reason[1] ^ lit;
				assert(other != lit);
				assert(isFalse(other));
				const uint32 other_v = ABS(other);
				CHECKVAR(other_v);
				assert(levels[other_v]);
				if (candlearnt || reason.original()) {
					if (subsumed(lit) && subsumed(other)) {
						candsubsumed = true;
						break;
					}
				}
				if (!seen[other_v]) {
					seen[other_v] = ANALYZED_M;
					analyzed.push(other);
				}
			}
			else {
				candsubsumed = subsumed(lit);
				forall_clause(reason, k) {
					const uint32 other = *k;
					if (other == lit) continue;
					assert(isFalse(other));
					assert(other != flit);
					const uint32 other_v = ABS(other);
					CHECKVAR(other_v);
					if (levels[other_v]) {
						if (candsubsumed && notsubsumed(other)) candsubsumed = false;
						if (!seen[other_v]) {
							seen[other_v] = ANALYZED_M;
							analyzed.push(other);
						}
					}
				}
				if (candsubsumed && (candlearnt || reason.original()))
					break;
				candsubsumed = false;
			}
		}
		else {
			LOG2(4, "  found decision %d", l2i(lit));
			learntC.push(flit);
		}
	}

	unmark_literals(cand);

	original = conflictoriginality;

	return candsubsumed;
}

bool Solver::vivifyLearn(CLAUSE& cand, const C_REF& cref, const int& nonFalse, const bool& original)
{
	assert(conflict < UNDEF_REF);
	assert(nonFalse > 2);
	assert(cand == cm[cref]);
	LOGLEARNT(this, 3);
	const int learntsize = learntC.size();
	assert(learntsize <= nonFalse);
	bool success;
	if (learntsize == 1) {
		LOG2(4, "  candidate is strengthened by a unit");
		backtrack();
		enqueueUnit(learntC[0]);
		removeClause(cand, cref);
		ignore = UNDEF_REF;
		if (BCPVivify()) {
			LOG2(2, "  propagating vivified unit proved a contradiction");
			learnEmpty();
		}
#ifdef STATISTICS
		stats.vivify.strengthened++;
#endif
		success = true;
	}
	else if (learntsize < nonFalse) {
		LOG2(4, "  candidate is strengthened");
		backtrack();
		if (opts.proof_en) proof.addClause(learntC);
		removeClause(cand, cref);
		sp->learntLBD = learntsize - 1;
		addClause(learntC, cand.learnt());
#ifdef STATISTICS
		stats.vivify.strengthened++;
#endif
		success = true;
	}
	else if (original && cand.original()) {
		assert(learntsize == nonFalse);
		LOG2(4, "  candidate is subsumed");
		removeClause(cand, cref);
#ifdef STATISTICS
		stats.vivify.subsumed++;
#endif
		success = true;
	}
	else {
		assert(learntsize == nonFalse);
		success = false;
	}
	return success;
}

bool Solver::vivifyClause(const C_REF& cref)
{
	assert(UNSOLVED);
	assert(!cm.deleted(cref));
	CLAUSE* candptr = cm.clause(cref);
	CLAUSE& candref = *candptr;
	LOGCLAUSE(4, candref, "  trying to vivify candidate");
	uint32* clause = sp->tmpstack;
	int tail = 0;
	LIT_ST* values = sp->value;
	forall_clause(candref, k) {
		const uint32 lit = *k;
		const LIT_ST val = values[lit];
		if (l2dl(lit) || UNASSIGNED(val)) {
			assert(uint32(tail) < inf.maxVar);
			clause[tail++] = lit;
		}
		else if (val) {
			assert(!l2dl(lit));
			LOG2(4, "  already satisfied by the root(%d)", l2i(lit));
			removeClause(candref, cref);
			return false;
		}
	}
	assert(tail > 1);
	assert(tail <= candref.size());
	if (tail == 2) return false; // skip non-false binaries
	uint32 unit = 0;
	forall_clause(candref, k) {
		const uint32 lit = *k;
		const LIT_ST val = values[lit];
		if (UNASSIGNED(val)) {
			unit = 0;
			break;
		}
		else if (val) { 
			if (unit) {
				unit = 0;
				break;
			}
			unit = lit;
			LOG2(4, "  found potential unit or decision %d", l2i(unit));
		}
	}
	if (unit) {
		CHECKLIT(unit);
		assert(l2dl(unit));
		assert(isTrue(unit));
		if (l2r(unit) == cref) {
#ifdef LOGGING
			LOG2(4, "  candidate is the reason of unit(%d)", l2i(unit));
			const uint32 level = l2dl(unit);
			assert(level > 0);
			LOG2(4, "  forced to backtrack to level %d", level - 1);
			backtrack(level - 1);
#else
			assert(l2dl(unit) > 0);
			backtrack(l2dl(unit) - 1);
#endif
		}
	}

	// begin vivification
	stats.vivify.checks++;

	pdqsort(clause, clause + tail, HIST_MCV_CMP(vhist));

	LOGSORTED(this, tail, 4);

	conflict = ignore = UNDEF_REF;

	uint32 level = 0;
	bool success = false;
	for (int i = 0; i < tail; ++i) {
		const uint32 lit = clause[i], fit = FLIP(lit);
		CHECKLIT(lit);
		if (level++ < LEVEL) {
			if (dlevel[level].decision == fit) {
#ifdef STATISTICS
				stats.vivify.reused++;
#endif
				assert(isFalse(lit));
				continue;
			}
			backtrack(level - 1);
		}
		const LIT_ST val = values[lit];
		assert(UNASSIGNED(val) || l2dl(lit) <= level);
		if (UNASSIGNED(val)) {
#ifdef STATISTICS
			stats.vivify.assumed++;
#endif
			enqueueDecision(fit);
			ignore = cref;
			bool hasConflict = false;
			if (LEVEL == 1) {
				hasConflict = BCPProbe();
				candptr = cm.clause(cref); // 'candptr' must be updated in case new hyper binaries are added
			}
			else
				hasConflict = BCPVivify();
			assert(cm.clause(cref) == candptr);
			if (hasConflict) {
				CLAUSE& _candref = *candptr;
				bool original = false;
				if (vivifyAnalyze(_candref, original)) {
					LOG2(4, "  candidate is subsumed by conflict/reason");
					removeClause(_candref, cref);
#ifdef STATISTICS
					stats.vivify.subsumed++;
#endif
					success = true;
				}
				else 
					success = vivifyLearn(_candref, cref, tail, original);
				clearVivify();
				break;
			}
		}
		else if (val && candptr->learnt()) {
			assert(l2dl(lit));
			assert(conflict == UNDEF_REF);
			assert(analyzed.empty());
			LOG2(4, "  candidate is a learnt implication already satisfied by %d", l2i(lit));
			removeClause(*candptr, cref);
#ifdef STATISTICS
			stats.vivify.implied++;
#endif
			success = true;
			break;
		}
	}
	return success;
}

void Solver::scheduleVivify(BCNF& cnf, const bool& tier2, const bool& learnt)
{
	int lowlbd = 0, highlbd = 0;
	if (tier2) { lowlbd = opts.lbd_tier1 + 1; highlbd = opts.lbd_tier2; }
	else if (learnt) highlbd = opts.lbd_tier1;
	uint32 prioritized = 0;
	uint32* hist = vhist.data();
	PREFETCH_CM(cs, deleted);
	if (learnt) {
		LOGN2(2, "  shrinking learnts before vivification..");

	#ifdef STATISTICS
		CNFLEARNTINF(beforeCls, beforeLits);
	#endif

		shrinkTop(learnts);

	#ifdef STATISTICS
		LOGSHRINKLEARNT(2, beforeCls, beforeLits);
	#endif

		for (CL_ST p = 0; p < 2; ++p) {
			const bool priority = p;
			forall_cnf(learnts, i) {
				const C_REF ref = *i;
				if (deleted[ref]) continue;
				GET_CLAUSE(c, ref, cs);
				if (c.hyper()) continue;
				if (c.lbd() < lowlbd) continue;
				if (c.lbd() > highlbd) continue;
				if (c.vivify() != priority) continue;
				assert(c.learnt());
				if (priority) prioritized++;
				hist_clause(c, hist);
				cnf.push(ref);
			}
		}
	}
	else {
		assert(!tier2);
		LOGN2(2, "  shrinking originals before vivification..");

	#ifdef STATISTICS
		CNFORGINF(beforeCls, beforeLits);
	#endif

		shrinkTop(orgs);

	#ifdef STATISTICS
		LOGSHRINKORG(2, beforeCls, beforeLits);
	#endif

		for (CL_ST p = 0; p < 2; ++p) {
			const bool priority = p;
			forall_cnf(orgs, i) {
				const C_REF ref = *i;
				if (deleted[ref]) continue;
				GET_CLAUSE(c, ref, cs);
				if (c.vivify() != priority) continue;
				if (priority) prioritized++;
				assert(c.original());
				hist_clause(c, hist);
				cnf.push(ref);
			}
		}
	}
	const uint32 scheduled = cnf.size();
	const char* ctype = learnt ? (tier2 ? "learnt-tier2" : "learnt-tier1") : "original";
	if (prioritized) {
		LOG2(2, " Vivification %lld: scheduler prioritized %d %s clauses %.2f%%", 
			stats.probe.calls, prioritized, ctype, percent(prioritized, scheduled));
	}
	else {
		LOGN2(2, " Vivification %lld: scheduler prioritizing all %s clauses..", stats.probe.calls, ctype);

		forall_cnf(cnf, i) { 
			GET_CLAUSE(c, *i, cs);
			c.markVivify();
		}

		LOGDONE(2, 5);
	}
	LOG2(2, " Vivification %lld: scheduled %d %s clauses %.2f%%",
		stats.probe.calls, scheduled, ctype, percent(scheduled, learnt ? learnts.size() : orgs.size()));
}

void Solver::vivifying(const CL_ST& type)
{
	assert(!LEVEL);
	assert(sp->propagated == trail.size());
	const bool tier1 = ISVIVIFYTIER1(type);
	const bool tier2 = ISVIVIFYTIER2(type);
	const bool learnt = tier2 || tier1;
	BCNF schedule;
	wt.clear(true);
	vhist.resize(inf.nDualVars);
	memset(vhist, 0, sizeof(uint32) * inf.nDualVars);
	scheduleVivify(schedule, tier2, learnt);
	sortVivify(schedule);
	rebuildWT(opts.vivify_priorbins);
	SET_BOUNDS(limit, vivify, probeticks, searchticks, nlogn(schedule.size()));
	if (tier2) {
		const uint64 extra = (limit - stats.probeticks) << 1;
		limit += extra;
		LOG2(2, "  %s tier2 efficiency bounds increased to %lld by an extra weight %lld", __func__, limit, extra);
	}
	uint32 vivified = 0, candidates = 0;
	while (schedule.size()
		&& NOT_UNSAT
		&& stats.probeticks <= limit
		&& !INTERRUPTED)
	{
		const C_REF ref = schedule.back();
		schedule.pop();
		if (cm.deleted(ref)) continue;
		candidates++;
		if (vivifyClause(ref)) vivified++;
		cm[ref].initVivify();
	}
	vhist.clear(true);
	schedule.clear(true);
	if (NOT_UNSAT) backtrack();
	stats.vivify.vivified += vivified;
	LOG2(2, " Vivification %lld: vivified %d %s clauses %.2f%% per %d candidates",
		stats.probe.calls, vivified, 
		learnt ? (tier2 ? "learnt-tier2" : "learnt-tier1") : "original",
		percent(vivified, candidates), candidates);
}

void Solver::vivify()
{
	if (UNSAT) return;
	assert(probed);
	assert(!LEVEL);
	if (!canVivify()) return;
	vivifying(VIVIFYTIER2);
	if (NOT_UNSAT) {
		vivifying(VIVIFYTIER1);
		const bool enough = (ORIGINALS >> 3) < LEARNTS;
		if (NOT_UNSAT && enough) vivifying(VIVIFYORG);
	}
	printStats(1, 'v', CVIOLET5);
}

inline bool	Solver::canVivify()
{
	if (!opts.vivify_en) return false;
	if (!LEARNTS) return false;
	return (ORIGINALS + (LEARNTS << 2)) 
		< (stats.searchticks + opts.vivify_min_eff);
}