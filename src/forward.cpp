/***********************************************************************[forward.cpp]
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

struct SUBSUME_RANK {
	inline uint32 operator () (const CSIZE& a) const { return a.size; }
};

inline void Solver::strengthen(CLAUSE& c, const uint32& self) 
{
	CHECKLIT(self);
	assert(c.size() > 2);
	assert(unassigned(self));
	if (opts.proof_en) proof.shrinkClause(c, self);
	uint32 *j = c;
	forall_clause(c, i) {
		const uint32 lit = *i;
		assert(unassigned(lit));
		if (NEQUAL(lit, self)) 
			*j++ = lit;
	}
	assert(j + 1 == c.end());
	shrinkClause(c, 1);
	c.initTier3();
}

inline void	Solver::removeSubsumed(CLAUSE& c, const C_REF& cref, CLAUSE* subsuming) 
{
	assert(subsuming->size() <= c.size());
	assert(c.size() > 2);
	if (c.original() && subsuming->learnt()) {
		subsuming->markOriginal();
		const int ssize = subsuming->size();
		stats.clauses.original++;
		assert(stats.clauses.learnt > 0);
		stats.clauses.learnt--;
		stats.literals.original += ssize;
		assert(stats.literals.learnt > 0);
		stats.literals.learnt -= ssize;
	}
	removeClause(c, cref);
}

inline bool Solver::subsumeCheck(CLAUSE* subsuming, uint32& self, const LIT_ST* marks)
{
	stats.forward.checks++;
	assert(!self);
	uint32 prev = 0;
	uint32* end = subsuming->end();
	bool good = true;
	for (uint32* i = subsuming->data(); good && i != end; ++i) {
		uint32 lit = *i;
		CHECKLIT(lit);
		assert(unassigned(lit));
		*i = prev;
		prev = lit;
		LIT_ST marker = marks[ABS(lit)];
		if (UNASSIGNED(marker)) good = false;
		else if (marker == SIGN(lit)) continue;
		else if (self) good = false;
		else self = lit;
	}
	assert(prev);
	assert(!**subsuming);
	**subsuming = prev;
	return good;
}

inline CL_ST Solver::subsumeClause(CLAUSE& c, const cbucket_t* cs, const bool* deleted, const State_t* states, const C_REF& cref)
{
	assert(!cm.deleted(cref));
	assert(c.size() > 2);
	assert(keeping(c));

	mark_literals(c);   

	const LIT_ST* marks = sp->marks;
	CLAUSE* s = NULL;
	uint32 self = 0;
	forall_clause(c, k) {
		const uint32 lit = *k;
		if (!states[ABS(lit)].subsume) continue;
		for (LIT_ST sign = 1; !s && sign >= 0; --sign) {
			assert(sign == 0 || sign == 1);
			const uint32 slit = sign ? FLIP(lit) : lit;
			BOL& others = bot[slit];
			forall_bol(others, o) {
				self = 0;
				const uint32 imp = *o;
				const LIT_ST marker = marks[ABS(imp)], impSign = SIGN(imp);
				if (UNASSIGNED(marker)) continue;
				if (marker && sign) continue; // tautology
				if (marker == !impSign) {
					if (sign) continue; // tautology
					self = imp;
				}
				else if (sign) self = slit;
				assert(subbin.original());
				assert(subbin.binary());
				subbin[0] = slit, subbin[1] = imp;
				s = &subbin; // "always original"
				break;
			}

			if (s) break;

			WOL& wol = wot[slit];
			forall_wol(wol, i) {
				const C_REF dref = *i;
				if (deleted[dref]) continue;
				GET_CLAUSE_PTR(d, dref, cs);
				assert(d != &c);
				assert(d->size() <= c.size());
				if (subsumeCheck(d, self, marks)) {
					s = d; // can be "original or learnt" 
					break;
				}
				else self = 0;
			}
		}

		if (s) break;
	}

	unmark_literals(c);

	if (!s) return 0;

	if (self) {
		LOGCLAUSE(3, c, "  candidate ");
		LOGCLAUSE(3, (*s), "  strengthened by ");
		strengthen(c, FLIP(self));
		return -1;
	}
	else {
		LOGCLAUSE(3, c, "  candidate ");
		LOGCLAUSE(3, (*s), "  subsumed by ");
		removeSubsumed(c, cref, s);
		return 1;
	}
}

void Solver::scheduleForward(BCNF& cnf, const cbucket_t* cs, const bool* deleted)
{
	if (cnf.empty()) return;

	const int maxsize = opts.forward_max_csize;
	const LIT_ST* values = sp->value;
	const State_t* states = sp->state;
	uint32* hist = vhist.data();

	forall_cnf(cnf, i) {
		const C_REF r = *i;
		if (deleted[r]) continue;
		GET_CLAUSE(c, r, cs);
		const int size = c.size();
		if (size > maxsize) continue;
		if (!keeping(c)) continue;
		// check marked-subsume/rooted literals
		int subsume = 0;
		bool rooted = false;
		forall_clause(c, k) {
			const uint32 lit = *k;
			CHECKLIT(lit);
			if (!UNASSIGNED(values[lit])) { 
				rooted = true; 
				break; 
			}
			else if (states[ABS(lit)].subsume) 
				subsume++;
			assert(values[lit] == UNDEF_VAL);
		}
		if (rooted || subsume < 2) continue;
		if (c.subsume()) 
			stats.forward.leftovers++;
		hist_clause(c, hist);
		scheduled.push(CSIZE(r, (uint32)size));
	}
}

void Solver::forwardAll()
{
	if (INTERRUPTED) killSolver();
	assert(!LEVEL);
	assert(inf.unassigned);
	assert(conflict == UNDEF_REF);
	assert(wt.empty());

	// schedule clauses
	PREFETCH_CM(cs, deleted);

	uint64 checked = 0, subsumed = 0, strengthened = 0;
	BCNF shrunken;

	stats.forward.leftovers = 0;

	vhist.resize(inf.nDualVars, 0);
	scheduled.reserve(MAXCLAUSES);

	scheduleForward(orgs, cs, deleted);
	scheduleForward(learnts, cs, deleted);

	if (scheduled.size()) {

		LOG2(2, " Scheduled %lu (%.2f %%) clauses for subsumption", 
			scheduled.size(), percent((double)scheduled.size(), (double)MAXCLAUSES));

		SET_BOUNDS(forward_limit, forward, forward.checks, searchprops, 0);
		
		radixSort(scheduled.data(), scheduled.end(), SUBSUME_RANK());

		const uint32 maxoccurs = opts.forward_max_occs;
		const State_t* states = sp->state;
		uint32* hist = vhist.data();

		if (!stats.forward.leftovers) {
			forall_vector(CSIZE, scheduled, i) {
				assert(i->ref < cm.size());
				GET_CLAUSE(c, i->ref, cs);
				if (c.size() > 2) 
					c.markSubsume();
			}
		}
		
		wot.resize(inf.nDualVars);
		bot.resize(inf.nDualVars);

		forall_vector(CSIZE, scheduled, i) {
			if (INTERRUPTED) break;
			if (stats.forward.checks >= forward_limit) break;

			checked++;

			const C_REF r = i->ref;
			GET_CLAUSE(c, r, cs);
			assert(!cm.deleted(r));

			LOGCLAUSE(4, c, "  subsuming ");
			if (c.size() > 2 && c.subsume()) {
				c.initSubsume();
				CL_ST st = subsumeClause(c, cs, deleted, states, r);
				if (st > 0) { subsumed++; continue; }
				if (st < 0) { shrunken.push(r); strengthened++; }
			}

			bool subsume = true, orgbin = (c.binary() && c.original());
			uint32 minlit = 0, minhist = 0, minsize = 0;
			forall_clause(c, k) {
				const uint32 lit = *k;
				if (!states[ABS(lit)].subsume) subsume = false;
				const uint32 currentsize = orgbin ? bot[lit].size() : wot[lit].size();
				if (minlit && minsize <= currentsize) continue;
				const uint32 h = hist[lit];
				if (minlit && minsize == currentsize && h <= minhist) continue;
				minlit = lit, minsize = currentsize, minhist = h;
			}

			// current scheduled clause cannot subsume more clauses
			if (!subsume) continue;

			// attach new occurrence
			if (minsize <= maxoccurs) {
				if (orgbin)
					bot[minlit].push(c[0] ^ c[1] ^ minlit);
				else
					wot[minlit].push(r);
			}
		}
	}

	LOG2(2, " Subsume %lld: removed %lld and strengthened %lld clauses", stats.forward.calls, subsumed, strengthened);

	if (scheduled.size() == checked) 
		sp->clearSubsume();

	forall_cnf(shrunken, i) { 
		GET_CLAUSE(c, *i, cs);
		mark_subsume(c);
	}

	wot.clear(true);
	bot.clear(true);
	vhist.clear(true);
	shrunken.clear(true);
	scheduled.clear(true);
	
	stats.forward.subsumed += subsumed;
	stats.forward.strengthened += strengthened;
}

void Solver::forward()
{
	if (!ORIGINALS && !LEARNTS) return;
	rootify();
	assert(UNSOLVED);
	stats.forward.calls++;
	printStats(1, '-', CORANGE0);
	wt.clear(true);
	forwardAll();
	rebuildWT(opts.forward_priorbins);
	filterOrg();
	if (retrail()) LOG2(2, " Propagation after subsume proved a contradiction");
	INCREASE_LIMIT(forward, stats.forward.calls, nlognlogn, true);
	printStats(true, 'u', CORANGE1);
}

void Solver::filterOrg() 
{
	const bool* deleted = cm.stencil();
	C_REF* j = learnts;
	forall_cnf(learnts, i) {
		const C_REF r = *i;
		if (deleted[r]) continue;
		if (cm[r].learnt()) *j++ = r;
		else orgs.push(r);
	}
	assert(j >= learnts);
	learnts.resize(uint32(j - learnts));
}