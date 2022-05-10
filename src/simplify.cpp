/***********************************************************************[simp.cpp]
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

#include "simplify.hpp"
#include "control.hpp"
#include "sort.hpp"
#include "can.hpp"

using namespace SeqFROST;

#define stopping(PHASE,DIFF) (PHASE == opts.phases) || (DIFF <= opts.phase_lits_min && PHASE > 2)

inline bool	Solver::checkMem(const string& name, const size_t& size)
{
	const size_t sysMemCons = size_t(sysMemUsed()) + size;
	if (sysMemCons > size_t(stats.sysmem)) { // to catch memout problems before exception does
		LOGWARN("not enough memory for %s (free: %lld, used: %zd), simplify will terminate",
			name.c_str(), stats.sysmem / MBYTE, sysMemCons / MBYTE);
		return false;
	}
	return true;
}

void Solver::createOT(const bool& reset)
{
	if (opts.profile_simplifier) timer.pstart();
	if (reset) {
		forall_variables(v) { 
			const uint32 p = V2L(v); 
			ot[p].clear(); 
			ot[NEG(p)].clear();
		}
	}
	forall_sclauses(scnf, i) {
		const S_REF r = *i;
		SCLAUSE& c = scnf[r];
		if (c.learnt() || c.original()) {
			assert(c.size());
			forall_clause(c, k) { 
				CHECKLIT(*k);
				ot[*k].push(r);
			}
		}
	}
	if (opts.profile_simplifier) timer.pstop(), timer.cot += timer.pcpuTime();
}

void Solver::reduceOL(OL& ol)
{
	if (ol.empty()) return;
	S_REF *j = ol;
	forall_occurs(ol, i) {
		if (scnf[*i].deleted()) continue;
		*j++ = *i;
	}
	ol.resize(int(j - ol));
}

void Solver::reduceOT()
{
	if (opts.profile_simplifier) timer.pstart();
	forall_variables(v) {
		const uint32 p = V2L(v), n = NEG(p);
		reduceOL(ot[p]);
		reduceOL(ot[n]);
	}
	if (opts.profile_simplifier) timer.pstop(), timer.rot += timer.pcpuTime();
}

void Solver::sortOT()
{
	if (opts.profile_simplifier) timer.pstart();
	CNF_CMP_KEY cmp(scnf);
	forall_vector(uint32, elected, i) {
		CHECKVAR(*i);
		const uint32 p = V2L(*i), n = NEG(p);
		OL& poss = ot[p], &negs = ot[n];
		const int ps = poss.size(), ns = negs.size();
		if (ps > 1) 
			SORTCMP(poss, cmp);
		if (ns > 1) 
			SORTCMP(negs, cmp);
	}
	if (opts.profile_simplifier) timer.pstop(), timer.sot += timer.pcpuTime();
}

void Solver::simplify()
{
	if (!opts.phases && !(opts.all_en || opts.ere_en)) return;
	assert(conflict == UNDEF_REF);
	assert(UNSOLVED);
	assert(ORIGINALS);
	stats.simplify.calls++;
	simplifying();
	INCREASE_LIMIT(simplify, stats.simplify.calls, nlognlogn, true);
	last.simplify.reduces = stats.reduces + 1;
	if (opts.phases > 2) {
		opts.phases--;
		LOG2(2, "  sigmify phases decreased to %d", opts.phases);
	}
}

void Solver::extract(BCNF& cnf)
{
	assert(SIMP_CLAUSESIZE == sizeof(SCLAUSE));
	assert(SIMP_CLAUSEBUCKETS == SIMP_CLAUSESIZE / sizeof(uint32));
	forall_cnf(cnf, i) {
		const C_REF ref = *i;
		if (cm.deleted(ref)) continue;
		const CLAUSE& src = cm[ref];
		const int size = src.size();
		SCLAUSE& dest = *scnf.alloc(src);
		dest.calcSig();
		SORT(dest);
		inf.nClauses++;
		inf.nLiterals += size;
	}
}

void Solver::awaken()
{ 
	printStats(1, '-', CGREEN0);
	initSimp();
	LOGN2(2, " Allocating memory..");
	wt.clear(true);
	ot.resize(inf.nDualVars);
	scnf.init(MAXCLAUSES, MAXLITERALS);
	LOGENDING(2, 5, "(%.1f MB used)", 
		(double(ot.capacity()) + double(scnf.capacity())) / MBYTE);
	LOGN2(2, " Extracting clauses to simplifying CNF..");
	inf.nClauses = inf.nLiterals = 0;
	extract(orgs), orgs.clear(true);
	extract(learnts), learnts.clear(true);
	cm.destroy();
	LOGENDING(2, 5, "(%d clauses extracted)", inf.nClauses);
	LOGMEMCALL(this, 2);
	return;
}

void Solver::simplifying()
{
	/********************************/
	/*        awaken simplify       */
	/********************************/
	SLEEPING(sleep.simplify, opts.simplify_sleep_en);
	rootify();
	shrinkTop(false);
	if (orgs.empty()) {
		PREFETCH_CM(cs, deleted);
		recycleWT(cs, deleted);
		return;
	}
	timer.stop();
	timer.solve += timer.cpuTime();
	if (!opts.profile_simplifier) timer.start();
	awaken();
	if (simpstate == AWAKEN_FAIL) {
		recycle();
		return;
	}
	if (INTERRUPTED) killSolver();
	/********************************/
	/*      V/C Eliminations        */
	/********************************/
	assert(!phase && !multiplier);
	int64 bmelted = inf.maxMelted, bclauses = inf.nClauses, bliterals = inf.nLiterals;
	int64 litsbefore = inf.nLiterals, diff = INT64_MAX;
	while (litsbefore) {
		resizeCNF();
		createOT();
		if (!prop()) killSolver();
		if (!LCVE()) break;
		sortOT();
		if (stopping(phase, diff)) { ERE(); break; }
		SUB(), VE(), BCE();
		countAll(), filterElected();
		inf.nClauses = inf.nClausesAfter, inf.nLiterals = inf.nLiteralsAfter;
		diff = litsbefore - inf.nLiterals, litsbefore = inf.nLiterals;
		phase++;
		multiplier++;
		multiplier += phase == opts.phases;
	}
	/********************************/
	/*          Write Back          */
	/********************************/
	// prop. remaining units if formula is empty
	// where recreating OT is not needed as there
	// are nothing to add
	if (!prop()) killSolver(); 
	assert(sp->propagated == trail.size());
	if (INTERRUPTED) killSolver();
	occurs.clear(true), ot.clear(true);
	clearMapFrozen();
	countFinal();
	shrinkSimp();
	assert(inf.nClauses == scnf.size());
	bool success = (bliterals != inf.nLiterals);
	stats.simplify.all.variables += int64(inf.maxMelted) - bmelted;
	stats.simplify.all.clauses += bclauses - int64(inf.nClauses);
	stats.simplify.all.literals += bliterals - int64(inf.nLiterals);
	last.shrink.removed = stats.shrunken;
	if (inf.maxFrozen > sp->simplified) stats.units.forced += inf.maxFrozen - sp->simplified;
	if (!inf.unassigned || !inf.nClauses) { 
		LOG2(2, " Formula is SATISFIABLE by elimination");
		SET_SAT; 
		printStats(1, 's', CGREEN); 
		return;
	}
	if (canMap()) map(true); 
	else newBeginning();
	rebuildWT(opts.simplify_priorbins);
	if (retrail()) LOG2(2, " Propagation after sigmify proved a contradiction");
	UPDATE_SLEEPER(simplify, success);
	printStats(1, 's', CGREEN);
	if (!opts.profile_simplifier) timer.stop(), timer.simplify += timer.cpuTime();
	if (!opts.solve_en) killSolver();
	timer.start();
}

void Solver::shrinkSimp() 
{
	if (opts.profile_simplifier) timer.start();
	SCNF new_scnf;
	new_scnf.init(inf.nClauses, inf.nLiterals);
	forall_sclauses(scnf, i) {
		const SCLAUSE& src = scnf[*i];
		if (src.deleted()) continue;
		new_scnf.alloc(src);
	}
	new_scnf.migrateTo(scnf);
	if (opts.profile_simplifier) timer.stop(), timer.gc += timer.cpuTime();
}

void Solver::newBeginning() 
{
	assert(opts.preprocess_en || opts.simplify_en);
	assert(wt.empty());
	assert(orgs.empty());
	assert(learnts.empty());
	assert(inf.nClauses == scnf.size());
	cm.init(inf.nClauses, inf.nLiterals);
	stats.literals.original = stats.literals.learnt = 0;
	if (opts.aggr_cnf_sort) 
		STABLESORT(scnf.refs().data(), scnf.end(), scnf.size(), STABLE_CNF_KEY(scnf));
	forall_sclauses(scnf, i) { 
		addClause(scnf[*i]);
	}
	stats.clauses.original = orgs.size();
	stats.clauses.learnt = learnts.size();
	assert(MAXCLAUSES == scnf.size());
	scnf.destroy();
}