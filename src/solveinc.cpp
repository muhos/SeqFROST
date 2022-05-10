/***********************************************************************[solveinc.cpp]
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
#include "version.hpp"

using namespace SeqFROST;

Solver::Solver() :
	  sp(NULL)
	, vsidsheap(ACTIV_CMP(vsids.scores))
	, chbheap(ACTIV_CMP(chb.scores))
	, vschedule(SCORS_CMP(wot))
	, bumped(0)
	, conflict(UNDEF_REF)
	, ignore(UNDEF_REF)
	, decisionlevel(0)
	, decheuristic(0)
	, interrupted(false)
	, incremental(true)
	, stable(false)
	, probed(false)
	, cnfstate(UNSOLVED_M)
	, simpstate(AWAKEN_SUCC)
	, mapped(false)
{
	LOGTITLE("Solver (Sequential Formal Reasoning On Satisfiability)", version());
	getCPUInfo(stats.sysmem);
	getBuildInfo();
	initSolver();
}

void Solver::iallocSpace()
{
	imarks.clear(true);
	if (sp->size() == size_t(inf.maxVar) + 1) return; // avoid allocation if 'maxVar' didn't change
	assert(inf.maxVar);
	assert(inf.orgVars == inf.maxVar);
	assert(vorg.size() == inf.maxVar + 1);
	assert(V2L(inf.maxVar + 1) == inf.nDualVars);
	assert(model.lits.size() == inf.maxVar + 1);
	assert(ilevel.size() == ivstate.size());
	assert(ilevel.size() == inf.maxVar + 1);
	assert(imarks.empty());
	vorg[0] = 0;
	model.lits[0] = 0;
	model.init(vorg);
	LOGN2(2, " Allocating fixed memory for %d variables..", inf.maxVar);
	SP* newSP = new SP(inf.maxVar + 1, opts.polarity);
	newSP->copyFrom(sp);
	delete sp;
	sp = newSP;
	if (opts.proof_en)
		proof.init(sp, vorg);
	ilevel.clear(true);
	ivalue.clear(true);
	ivstate.clear(true);
	LOGDONE(2, 5);
	LOGMEMCALL(this, 2);
}

void Solver::isolve(Lits_t& assumptions)
{
	timer.start();
	iallocSpace();
	iunassume();
	assert(UNSOLVED);
	if (BCP()) {
		LOG2(2, " Incremental formula has a contradiction on top level");
		learnEmpty();
	}
	else {
		initLimits();
		iassume(assumptions);
		if (verbose == 1) printTable();
		if (canPreSigmify()) sigmify();
		if (UNSOLVED) {
			LOG2(2, "-- Incremental CDCL search started..");
			MDMInit();
			while (UNSOLVED && !INTERRUPTED) {
				if (BCP()) analyze();
				else if (!inf.unassigned) SET_SAT;
				else if (canReduce()) reduce();
				else if (canRestart()) restart();
				else if (canRephase()) rephase();
				else if (canSigmify()) sigmify();
				else if (canProbe()) probe();
				else if (canMMD()) MDM();
				else idecide();
			}
			LOG2(2, "-- Incremental CDCL search completed successfully");
		}
	}
	timer.stop();
	timer.solve += timer.cpuTime();
	wrapup();
}