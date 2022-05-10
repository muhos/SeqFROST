/***********************************************************************[solve.cpp]
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
#include "solve.hpp" 
#include "control.hpp"

namespace SeqFROST {
	CNF_INFO inf;
	Solver* solver = NULL;
}

using namespace SeqFROST;

Solver::Solver(const string& path) :
	formula(path)
	, sp(NULL)
	, vsidsheap(ACTIV_CMP(vsids.scores))
	, chbheap(ACTIV_CMP(chb.scores))
	, vschedule(SCORS_CMP(wot))
	, bumped(0)
	, conflict(UNDEF_REF)
	, ignore(UNDEF_REF)
	, decisionlevel(0)
	, decheuristic(0)
	, interrupted(false)
	, incremental(false)
	, stable(false)
	, probed(false)
	, cnfstate(UNSOLVED_M)
	, simpstate(AWAKEN_SUCC)
	, mapped(false)
{
	getCPUInfo(stats.sysmem);
	getBuildInfo();
	initSolver();
	if (!parser() || BCP()) { learnEmpty(), killSolver(); }
	if (opts.parseonly_en) killSolver();
}

void Solver::allocSolver()
{
	LOGN2(2, " Allocating fixed memory for %d variables..", inf.maxVar);
	assert(sizeof(LIT_ST) == 1);
	assert(inf.maxVar);
	assert(sp == NULL);
	const uint32 maxSize = inf.maxVar + 1;
	sp = new SP(maxSize, opts.polarity);
	cm.init(inf.orgCls, inf.nDualVars);
	wt.resize(inf.nDualVars);
	trail.reserve(inf.maxVar);
	dlevel.reserve(inf.maxVar);
	bumps.resize(maxSize, 0);
	vsids.scores.resize(maxSize, 0);
	chb.conflicts.resize(maxSize);
	chb.scores.resize(maxSize, 0.0);
	LOGDONE(2, 5);
	LOGMEMCALL(this, 2);
}

void Solver::initSolver()
{
	assert(UNDEFINED < 0 && UNDEF_VAL < 0);
	assert(!ORIGINAL && LEARNT && DELETED);
	assert(FROZEN_M && MELTED_M && SUBSTITUTED_M);
	assert(UNSAT_M == 0 && SAT_M == 1 && UNSOLVED_M == 2);
	assert(UNSOLVED);
	assert(sizeof(WATCH) >> 4 == 1);

	forceFPU();
	opts.init();
	subbin.resize(2);
	dlevel.push(level_t());

	if (opts.proof_en)
		proof.handFile(opts.proof_path, opts.proof_nonbinary_en);
}

void Solver::initLimits() 
{
	LOG2(2, " Initializing solver limits and heuristics..");

	//====================================
	// heuristics
	//====================================
	formula.c2v = ratio(double(ORIGINALS), double(inf.maxVar));
	if (opts.ternary_en && formula.ternaries < 2) {
		LOG2(2, "  Disabling hyper ternary resolution as no ternaries found");
		opts.ternary_en = false;
	}

	stable = opts.stable == 2;
	decheuristic = opts.decheuristic;

	vsids.init(opts.var_inc, opts.var_decay);
	mab.init(opts.mab_constant, opts.decheuristic);
	chb.init(opts.chb_step, opts.chb_step_decay, opts.chb_step_min);
	lbdrest.init(opts.lbd_rate, opts.lbd_fast, opts.lbd_slow);
	lbdrest.reset();

	if (stable) {
		LOG2(2, "  Initial stable mode is assumed");
		lubyrest.enable(opts.luby_inc, opts.luby_max);
	}
	else {
		LOG2(2, "  Initial unstable mode is assumed");
		updateUnstableLimit();
	}

	if (opts.seed) {
		LOG2(2, "  Initial random seed is set to %d", opts.seed);
		random.init(opts.seed);
	}

	//====================================
	// limits
	//====================================
	last.transitive.literals = 2;

	if (opts.stable == 1) {
		assert(!stable);
		INIT_LIMIT(limit.mode.conflicts, opts.mode_inc, false);
	}

	INIT_LIMIT(limit.reduce, opts.reduce_inc, false);
	INIT_LIMIT(limit.rephase, opts.rephase_inc, false);
	INIT_LIMIT(limit.mdm, opts.mdm_inc, true);
	INIT_LIMIT(limit.probe, opts.probe_inc, true);
	INIT_LIMIT(limit.simplify, opts.simplify_inc, true);
	INIT_LIMIT(limit.forward, opts.forward_inc, true);

	if (opts.mdm_rounds) {
		LOG2(2, "  Enabling MDM with %d rounds", opts.mdm_rounds);
		last.mdm.rounds = opts.mdm_rounds;
	}

	LOG2(2, " Limits initialized successfully");
}

void Solver::solve()
{
	FAULT_DETECTOR;
	timer.start();
	initLimits();
	if (verbose == 1) printTable();
	if (canPreSimplify()) simplify();
	if (UNSOLVED) {
		LOG2(2, "-- CDCL search started..");
		MDMInit();
		while (UNSOLVED && !EXHAUSTED) {
			if (BCP()) analyze();
			else if (!inf.unassigned) SET_SAT;
			else if (canReduce()) reduce();
			else if (canRestart()) restart();
			else if (canRephase()) rephase();
			else if (canSimplify()) simplify();
			else if (canProbe()) probe();
			else if (canMMD()) MDM();
			else decide();
		}
		LOG2(2, "-- CDCL search completed successfully");
	}
	timer.stop();
	timer.solve += timer.cpuTime();
	wrapup();
}

void Solver::wrapup() 
{
	if (!quiet_en) LOGHEADLINE(Result, CREPORT);
	if (quiet_en && opts.time_quiet_en) 
		LOG1("CPU time: %.3f  sec", timer.solve + timer.simplify);
	if (SAT) {
		LOGSAT("SATISFIABLE");
		assert(sp != NULL && sp->value != NULL);
		if (opts.model_en) {
			model.extend(sp->value);
			if (opts.modelprint_en) 
				model.print();
		}
		if (opts.modelverify_en) {
			model.extend(sp->value);
			model.verify(formula.path);
		}
	}
	else if (UNSAT) 
		LOGSAT("UNSATISFIABLE");
	else if (UNSOLVED) 
		LOGSAT("UNKNOWN");
	if (opts.proof_en) proof.close();
	if (opts.report_en) report();
}