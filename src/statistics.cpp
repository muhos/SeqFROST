/***********************************************************************[statistics.cpp]
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
#include "control.hpp"
using namespace SeqFROST;

void Solver::report()
{
	if (opts.report_en) {
		LOG0("");
		LOGHEADLINE(Statistics, CREPORT);
		LOG0("");
		if (opts.preprocess_en || opts.inprocess_en) {
			LOG1("\t\t\t%sSimplifier Report%s", CREPORT, CNORMAL);
			if (!opts.profile_simplifier)
				LOG1(" %sSimplifier time          : %s%.3f  seconds%s", CREPORT, CREPORTVAL, timer.simplify, CNORMAL);
			else {
				LOG1(" %s - Var ordering          : %s%-16.2f  ms%s", CREPORT, CREPORTVAL, timer.vo, CNORMAL);
				LOG1(" %s - CNF compact           : %s%-16.2f  ms%s", CREPORT, CREPORTVAL, timer.gc, CNORMAL);
				LOG1(" %s - OT  creation          : %s%-16.2f  ms%s", CREPORT, CREPORTVAL, timer.cot, CNORMAL);
				LOG1(" %s - OT  sorting           : %s%-16.2f  ms%s", CREPORT, CREPORTVAL, timer.sot, CNORMAL);
				LOG1(" %s - OT  reduction         : %s%-16.2f  ms%s", CREPORT, CREPORTVAL, timer.rot, CNORMAL);
				LOG1(" %s - BVE                   : %s%-16.2f  ms%s", CREPORT, CREPORTVAL, timer.ve, CNORMAL);
				LOG1(" %s - HSE                   : %s%-16.2f  ms%s", CREPORT, CREPORTVAL, timer.sub, CNORMAL);
				LOG1(" %s - BCE                   : %s%-16.2f  ms%s", CREPORT, CREPORTVAL, timer.bce, CNORMAL);
				LOG1(" %s - ERE                   : %s%-16.2f  ms%s", CREPORT, CREPORTVAL, timer.ere, CNORMAL);
			}
			LOG1(" %sSimplification calls     : %s%-10d%s", CREPORT, CREPORTVAL, stats.inprocess.calls, CNORMAL);
#ifdef STATISTICS
			LOG1(" %s Removed variables       : %s%-16lld%s", CREPORT, CREPORTVAL, stats.inprocess.all.variables + stats.units.forced, CNORMAL);
			LOG1(" %s  Resolutions            : %s%-16lld%s", CREPORT, CREPORTVAL, stats.inprocess.bve.resolutions, CNORMAL);
			LOG1(" %s  Forced units           : %s%-10d%s", CREPORT, CREPORTVAL, stats.units.forced, CNORMAL);
			LOG1(" %s  Pure literals          : %s%-16lld%s", CREPORT, CREPORTVAL, stats.inprocess.bve.pures, CNORMAL);
			LOG1(" %s  If-Then-Else           : %s%-16lld%s", CREPORT, CREPORTVAL, stats.inprocess.bve.ites, CNORMAL);
			LOG1(" %s  Inverter               : %s%-16lld%s", CREPORT, CREPORTVAL, stats.inprocess.bve.inverters, CNORMAL);
			LOG1(" %s  AND-OR                 : %s%-16lld%s", CREPORT, CREPORTVAL, stats.inprocess.bve.andors, CNORMAL);
			LOG1(" %s  Fun                    : %s%-16lld%s", CREPORT, CREPORTVAL, stats.inprocess.bve.funs, CNORMAL);
			LOG1(" %s  XOR                    : %s%-16lld%s", CREPORT, CREPORTVAL, stats.inprocess.bve.xors, CNORMAL);
			LOG1(" %s Removed clauses         : %s%-16lld%s", CREPORT, CREPORTVAL, stats.inprocess.all.clauses, CNORMAL);
			LOG1(" %s  Subsumed               : %s%-16lld%s", CREPORT, CREPORTVAL, stats.inprocess.sub.subsumed, CNORMAL);
			LOG1(" %s  Strengthened           : %s%-16lld%s", CREPORT, CREPORTVAL, stats.inprocess.sub.strengthened, CNORMAL);
			LOG1(" %s Tried redundancies      : %s%-16lld%s", CREPORT, CREPORTVAL, stats.inprocess.ere.tried, CNORMAL);
			LOG1(" %s  Original removed       : %s%-16lld%s", CREPORT, CREPORTVAL, stats.inprocess.ere.orgremoved, CNORMAL);
			LOG1(" %s  Original strengthened  : %s%-16lld%s", CREPORT, CREPORTVAL, stats.inprocess.ere.orgstrengthened, CNORMAL);
			LOG1(" %s  Learnt removed         : %s%-16lld%s", CREPORT, CREPORTVAL, stats.inprocess.ere.learntremoved, CNORMAL);
			LOG1(" %s  Learnt strengthened    : %s%-16lld%s", CREPORT, CREPORTVAL, stats.inprocess.ere.learntstrengthened, CNORMAL);
			LOG1(" %s Removed literals        : %s%-16lld%s", CREPORT, CREPORTVAL, stats.inprocess.all.literals, CNORMAL);
#else
			LOG1(" %sRemoved variables       : %s%-16lld%s", CREPORT, CREPORTVAL, stats.inprocess.all.variables + stats.units.forced, CNORMAL);
			LOG1(" %s Forced units           : %s%-10d%s", CREPORT, CREPORTVAL, stats.units.forced, CNORMAL);
			LOG1(" %sRemoved clauses         : %s%-16lld%s", CREPORT, CREPORTVAL, stats.inprocess.all.clauses, CNORMAL);
			LOG1(" %sRemoved literals        : %s%-16lld%s", CREPORT, CREPORTVAL, stats.inprocess.all.literals, CNORMAL);
#endif
		}
		LOG1("\t\t\t%sSolver Report%s", CREPORT, CNORMAL);
		LOG1(" %sSolver time              : %s%.3f  seconds%s", CREPORT, CREPORTVAL, timer.solve, CNORMAL);
		LOG1(" %sSystem memory            : %s%.3f  MB%s", CREPORT, CREPORTVAL, ratio(double(sysMemUsed()), double(MBYTE)), CNORMAL);
		LOG1(" %sFormula                  : %s%s%s", CREPORT, CREPORTVAL, formula.path.c_str(), CNORMAL);
		LOG1(" %s Size                    : %s%.3f  MB%s", CREPORT, CREPORTVAL, ratio(double(formula.size), double(MBYTE)), CNORMAL);
		LOG1(" %s Units                   : %s%-10d%s", CREPORT, CREPORTVAL, formula.units, CNORMAL);
		LOG1(" %s Binaries                : %s%-10d%s", CREPORT, CREPORTVAL, formula.binaries, CNORMAL);
		LOG1(" %s Ternaries               : %s%-10d%s", CREPORT, CREPORTVAL, formula.ternaries, CNORMAL);
		LOG1(" %s Larger                  : %s%-10d%s", CREPORT, CREPORTVAL, formula.large, CNORMAL);
		LOG1(" %s Max clause size         : %s%-10d%s", CREPORT, CREPORTVAL, formula.maxClauseSize, CNORMAL);
		LOG1(" %s C2V ratio               : %s%-10.3f%s", CREPORT, CREPORTVAL, formula.c2v, CNORMAL);
		if (opts.proof_en)
			LOG1(" %s Proof lines             : %s%-16zd%s", CREPORT, CREPORTVAL, proof.numClauses(), CNORMAL);
		LOG1(" %sAutarky calls            : %s%-16lld%s", CREPORT, CREPORTVAL, stats.autarky.calls, CNORMAL);
		LOG1(" %s Removed variables       : %s%-16lld%s", CREPORT, CREPORTVAL, stats.autarky.eliminated, CNORMAL);
		LOG1(" %sBacktracks               : %s%-16lld%s", CREPORT, CREPORTVAL, stats.backtrack.chrono + stats.backtrack.nonchrono, CNORMAL);
		LOG1(" %s Chronological           : %s%-16lld%s", CREPORT, CREPORTVAL, stats.backtrack.chrono, CNORMAL);
		LOG1(" %s Non-Chronological       : %s%-16lld%s", CREPORT, CREPORTVAL, stats.backtrack.nonchrono, CNORMAL);
		LOG1(" %s Trail reuses            : %s%-16lld%s", CREPORT, CREPORTVAL, stats.reuses, CNORMAL);
		LOG1(" %sConflicts                : %s%-16lld%s", CREPORT, CREPORTVAL, stats.conflicts, CNORMAL);
		LOG1(" %s OTF strengthened        : %s%-16lld%s", CREPORT, CREPORTVAL, stats.forward.strengthenedfly, CNORMAL);
		LOG1(" %s OTF subsumed            : %s%-16lld%s", CREPORT, CREPORTVAL, stats.forward.subsumedfly, CNORMAL);
		LOG1(" %s Learnt OTF subsumes     : %s%-16lld%s", CREPORT, CREPORTVAL, stats.subtried, CNORMAL);
		LOG1(" %s Learnt OTF subsumed     : %s%-16lld%s", CREPORT, CREPORTVAL, stats.forward.learntfly, CNORMAL);
		LOG1(" %s Learnt units            : %s%-10d%s", CREPORT, CREPORTVAL, stats.units.learnt, CNORMAL);
		LOG1(" %s Learnt literals         : %s%-16lld%s", CREPORT, CREPORTVAL, LEARNTLITERALS, CNORMAL);
		LOG1(" %s VMTF bumps              : %s%lld  (%0.3f %%)%s", CREPORT, CREPORTVAL, bumped, percent(double(bumped), double(UINT64_MAX)), CNORMAL);
		LOG1(" %s Alluip shrunken         : %s%-16lld%s", CREPORT, CREPORTVAL, stats.alluip.shrunken, CNORMAL);
#ifdef STATISTICS
		LOG1(" %s Minimized literals      : %s%2.2f %%%s", CREPORT, CREPORTVAL, percent((double)stats.minimize.before - stats.minimize.after, (double)stats.minimize.before), CNORMAL);
#endif
		LOG1(" %sDeduplications           : %s%-16lld%s", CREPORT, CREPORTVAL, stats.debinary.calls, CNORMAL);
		LOG1(" %s Hyper unaries           : %s%-16lld%s", CREPORT, CREPORTVAL, stats.debinary.hyperunary, CNORMAL);
		LOG1(" %s Duplicated binaries     : %s%-16lld%s", CREPORT, CREPORTVAL, stats.debinary.binaries, CNORMAL);
		LOG1(" %sDecompositions           : %s%-16lld%s", CREPORT, CREPORTVAL, stats.decompose.calls, CNORMAL);
		LOG1(" %s SCCs                    : %s%-16lld%s", CREPORT, CREPORTVAL, stats.decompose.scc, CNORMAL);
		LOG1(" %s Hyper unaries           : %s%-16lld%s", CREPORT, CREPORTVAL, stats.decompose.hyperunary, CNORMAL);
		LOG1(" %s Removed variables       : %s%-16lld%s", CREPORT, CREPORTVAL, stats.decompose.variables, CNORMAL);
		LOG1(" %s Removed clauses         : %s%-16lld%s", CREPORT, CREPORTVAL, stats.decompose.clauses, CNORMAL);
		LOG1(" %sForward calls            : %s%-16lld%s", CREPORT, CREPORTVAL, stats.forward.calls, CNORMAL);
		LOG1(" %s Checks                  : %s%-16lld%s", CREPORT, CREPORTVAL, stats.forward.checks, CNORMAL);
		LOG1(" %s Subsumed                : %s%-16lld%s", CREPORT, CREPORTVAL, stats.forward.subsumed, CNORMAL);
		LOG1(" %s Strengthened            : %s%-16lld%s", CREPORT, CREPORTVAL, stats.forward.strengthened, CNORMAL);
		LOG1(" %sHyper binary resolves    : %s%-16lld%s", CREPORT, CREPORTVAL, stats.binary.resolutions, CNORMAL);
		LOG1(" %s Added binaries          : %s%-16lld%s", CREPORT, CREPORTVAL, stats.binary.resolvents, CNORMAL);
		LOG1(" %sHyper ternary resolves   : %s%-16lld%s", CREPORT, CREPORTVAL, stats.ternary.resolutions, CNORMAL);
		LOG1(" %s Added binaries          : %s%-16lld%s", CREPORT, CREPORTVAL, stats.ternary.binaries, CNORMAL);
		LOG1(" %s Added ternaries         : %s%-16lld%s", CREPORT, CREPORTVAL, stats.ternary.ternaries, CNORMAL);
		LOG1(" %s Subsumed ternaries      : %s%-16lld%s", CREPORT, CREPORTVAL, stats.ternary.binaries * 2, CNORMAL);
		LOG1(" %sReduces                  : %s%-16lld%s", CREPORT, CREPORTVAL, stats.reduces, CNORMAL);
#ifdef STATISTICS
		LOG1(" %s Removed binaries        : %s%-16lld%s", CREPORT, CREPORTVAL, stats.binary.reduced, CNORMAL);
		LOG1(" %s Removed ternaries       : %s%-16lld%s", CREPORT, CREPORTVAL, stats.ternary.reduced, CNORMAL);
#endif
		LOG1(" %sRestarts                 : %s%-16lld%s", CREPORT, CREPORTVAL, stats.restart.all, CNORMAL);
		LOG1(" %s Stable restarts         : %s%-16lld%s", CREPORT, CREPORTVAL, stats.restart.stable, CNORMAL);
		LOG1(" %s Stable modes            : %s%-16lld%s", CREPORT, CREPORTVAL, stats.stablemodes, CNORMAL);
		LOG1(" %s Unstable modes          : %s%-16lld%s", CREPORT, CREPORTVAL, stats.unstablemodes, CNORMAL);
		LOG1(" %sRephases                 : %s%-16lld%s", CREPORT, CREPORTVAL, stats.rephase.all, CNORMAL);
#ifdef STATISTICS
		LOG1(" %s Original                : %s%-16lld%s", CREPORT, CREPORTVAL, stats.rephase.org, CNORMAL);
		LOG1(" %s Random                  : %s%-16lld%s", CREPORT, CREPORTVAL, stats.rephase.random, CNORMAL);
		LOG1(" %s Invert                  : %s%-16lld%s", CREPORT, CREPORTVAL, stats.rephase.inv, CNORMAL);
		LOG1(" %s Flip                    : %s%-16lld%s", CREPORT, CREPORTVAL, stats.rephase.flip, CNORMAL);
		LOG1(" %s Best                    : %s%-16lld%s", CREPORT, CREPORTVAL, stats.rephase.best, CNORMAL);
#endif
		LOG1(" %s Walk                    : %s%-16lld%s", CREPORT, CREPORTVAL, stats.walk.calls, CNORMAL);
		LOG1(" %sRecyclings               : %s%-16lld%s", CREPORT, CREPORTVAL, stats.recycle.soft + stats.recycle.hard, CNORMAL);
		LOG1(" %s Soft                    : %s%-16lld%s", CREPORT, CREPORTVAL, stats.recycle.soft, CNORMAL);
		LOG1(" %s Hard                    : %s%-16lld%s", CREPORT, CREPORTVAL, stats.recycle.hard, CNORMAL);
#ifdef STATISTICS
		LOG1(" %s Memory saved            : %s%.3f  MB%s", CREPORT, CREPORTVAL, ratio((double) stats.recycle.saved, (double) MBYTE) , CNORMAL);
#endif
		LOG1(" %sProbes calls             : %s%-16lld%s", CREPORT, CREPORTVAL, stats.probe.calls, CNORMAL);
		LOG1(" %s Rounds                  : %s%-16lld%s", CREPORT, CREPORTVAL, stats.probe.rounds, CNORMAL);
		LOG1(" %s Probed                  : %s%-16lld%s", CREPORT, CREPORTVAL, stats.probe.probed, CNORMAL);
		LOG1(" %s Failed                  : %s%-16lld%s", CREPORT, CREPORTVAL, stats.probe.failed, CNORMAL);
		LOG1(" %s Ticks                   : %s%-16lld%s", CREPORT, CREPORTVAL, stats.probeticks, CNORMAL);
		LOG1(" %sTransitive calls         : %s%-16lld%s", CREPORT, CREPORTVAL, stats.probe.calls, CNORMAL);
		LOG1(" %s Probed                  : %s%-16lld%s", CREPORT, CREPORTVAL, stats.transitive.probed, CNORMAL);
		LOG1(" %s Failed                  : %s%-16lld%s", CREPORT, CREPORTVAL, stats.transitive.failed, CNORMAL);
		LOG1(" %s Transitives             : %s%-16lld%s", CREPORT, CREPORTVAL, stats.transitive.removed, CNORMAL);
		LOG1(" %s Ticks                   : %s%-16lld%s", CREPORT, CREPORTVAL, stats.transitiveticks, CNORMAL);
#ifdef STATISTICS
		LOG1(" %sShrinks                  : %s%-16lld%s", CREPORT, CREPORTVAL, stats.shrink.calls, CNORMAL);
		LOG1(" %s removed clauses         : %s%-16lld%s", CREPORT, CREPORTVAL, stats.shrink.clauses, CNORMAL);
		LOG1(" %s removed literals        : %s%-16lld%s", CREPORT, CREPORTVAL, stats.shrink.literals, CNORMAL);
#endif
		LOG1(" %sSearch decisions         : %s%-16lld%s", CREPORT, CREPORTVAL, stats.decisions.single, CNORMAL);
		LOG1(" %s Propagations            : %s%-16lld%s", CREPORT, CREPORTVAL, stats.searchprops, CNORMAL);
		LOG1(" %s Ticks                   : %s%-16lld%s", CREPORT, CREPORTVAL, stats.searchticks, CNORMAL);
		LOG1(" %sMAB restarts             : %s%-16lld%s", CREPORT, CREPORTVAL, stats.mab.restarts, CNORMAL);
		LOG1(" %s VSIDS uses              : %s%-10d%s", CREPORT, CREPORTVAL, mab.select[0], CNORMAL);
		LOG1(" %s CHB uses                : %s%-10d%s", CREPORT, CREPORTVAL, mab.select[1], CNORMAL);
		LOG1(" %sMarker resets            : %s%-10d%s", CREPORT, CREPORTVAL, stats.markerresets, CNORMAL);
		LOG1(" %s Markings                : %s%d  (%0.3f %%)%s", CREPORT, CREPORTVAL, stats.marker, percent(double(stats.marker), double(UINT32_MAX)), CNORMAL);
		LOG1(" %sMapping calls            : %s%-10d%s", CREPORT, CREPORTVAL, stats.mapping.calls, CNORMAL);
		LOG1(" %s Variables compressed    : %s%-10d%s", CREPORT, CREPORTVAL, stats.mapping.compressed, CNORMAL);
		LOG1(" %sMDM calls                : %s%-10d%s", CREPORT, CREPORTVAL, stats.mdm.calls, CNORMAL);
		LOG1(" %s Walks                   : %s%-10d%s", CREPORT, CREPORTVAL, stats.mdm.walks, CNORMAL);
		LOG1(" %s VMTF uses               : %s%-10d%s", CREPORT, CREPORTVAL, stats.mdm.vmtf, CNORMAL);
		LOG1(" %s VSIDS uses              : %s%-10d%s", CREPORT, CREPORTVAL, stats.mdm.vsids, CNORMAL);
		LOG1(" %s CHB uses                : %s%-10d%s", CREPORT, CREPORTVAL, stats.mdm.chb, CNORMAL);
		LOG1(" %s All decisions           : %s%-16lld%s", CREPORT, CREPORTVAL, stats.decisions.multiple, CNORMAL);
		LOG1(" %s Assumed decisions       : %s%-16lld%s", CREPORT, CREPORTVAL, stats.decisions.massumed, CNORMAL);
		LOG1(" %s Last-made decisions     : %s%-10d%s", CREPORT, CREPORTVAL, last.mdm.decisions, CNORMAL);
		LOG1(" %sVivification checks      : %s%-16lld%s", CREPORT, CREPORTVAL, stats.vivify.checks, CNORMAL);
		LOG1(" %s Vivified                : %s%-16lld%s", CREPORT, CREPORTVAL, stats.vivify.vivified, CNORMAL);
#ifdef STATISTICS
		LOG1(" %s Reused                  : %s%-16lld%s", CREPORT, CREPORTVAL, stats.vivify.reused, CNORMAL);
		LOG1(" %s Assumed                 : %s%-16lld%s", CREPORT, CREPORTVAL, stats.vivify.assumed, CNORMAL);
		LOG1(" %s Implied                 : %s%-16lld%s", CREPORT, CREPORTVAL, stats.vivify.implied, CNORMAL);
		LOG1(" %s Subsumed                : %s%-16lld%s", CREPORT, CREPORTVAL, stats.vivify.subsumed, CNORMAL);
		LOG1(" %s Strengthened            : %s%-16lld%s", CREPORT, CREPORTVAL, stats.vivify.strengthened, CNORMAL);
#endif
		LOG1(" %sWalk calls               : %s%-16lld%s", CREPORT, CREPORTVAL, stats.walk.calls, CNORMAL);
		LOG1(" %s Checks                  : %s%-16lld%s", CREPORT, CREPORTVAL, stats.walk.checks, CNORMAL);
		LOG1(" %s Minimum                 : %s%-16lld%s", CREPORT, CREPORTVAL, stats.walk.minimum, CNORMAL);
		LOG1(" %s Flipped                 : %s%-16lld%s", CREPORT, CREPORTVAL, stats.walk.flipped, CNORMAL);
		LOG1(" %s Improved                : %s%-16lld%s", CREPORT, CREPORTVAL, stats.walk.improved, CNORMAL);
	}
}