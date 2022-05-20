/***********************************************************************[solve.hpp]
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

#ifndef __SOLVE_
#define __SOLVE_

#include "map.hpp"
#include "walk.hpp"
#include "heap.hpp"
#include "queue.hpp"
#include "model.hpp"
#include "proof.hpp"
#include "level.hpp"
#include "limit.hpp"
#include "timer.hpp"
#include "dimacs.hpp"
#include "random.hpp"
#include "restart.hpp"
#include "options.hpp"
#include "simptypes.hpp"
#include "solvetypes.hpp"
#include "statistics.hpp"
#include "heuristics.hpp"
#include "minimizeall.hpp"

namespace SeqFROST {

	#define NOT_UNSAT	cnfstate

	#define UNSAT		(!cnfstate)

	#define SAT			(cnfstate & SAT_M)

	#define UNSOLVED	(cnfstate & UNSOLVED_M)

	#define SET_SAT		(cnfstate = SAT_M)

	#define SET_UNSAT	(cnfstate = UNSAT_M)

	#define RESETSTATE	(cnfstate = UNSOLVED_M)

	#define INTERRUPTED	interrupted

	#define LEVEL	decisionlevel

	#define INCLEVEL(LIT) { decisionlevel = dlevel.size(); \
							dlevel.push(level_t(trail.size(), LIT)); }

	#define EXHAUSTED (INTERRUPTED || (opts.boundsearch_en && \
									  (stats.conflicts >= opts.conflict_out || \
									   stats.decisions.single >= opts.decision_out))) \

	#define TARGETING	(stable || opts.targetonly_en)

	#define HEAPSCORES	(decheuristic ? chb.scores : vsids.scores)

	#define DECISIONHEAP (decheuristic ? chbheap : vsidsheap)


	class Solver {
	protected:
		FORMULA			formula;
		TIMER			timer;
		CMM				cm;
		WT				wt;
		SP				*sp;
		LIMIT			limit;
		LAST			last;
		STATS			stats;
		SLEEP			sleep;
		BCNF			orgs;
		BCNF			learnts;
		BCNF			reduced;
		CLAUSE			subbin;
		MAB				mab;
		CHB				chb;
		VSIDS			vsids;
		QUEUE			vmtf;
		MAP				vmap;
		Lits_t			learntC;
		bump_t			bumps;
		dheap_t			vsidsheap;
		dheap_t			chbheap;
		dlevel_t		dlevel;
		vsched_t		vschedule;
		csched_t		scheduled;
		Vec<OCCUR>		occurs;
		Vec<DWATCH>		dwatches;
		Vec<WOL>		wot;
		Vec<BOL>		bot;	
		uVec1D			lbdlevels;
		uVec1D			eligible;
		uVec1D			probes;
		uVec1D			trail;
		uVec1D			vorg;
		uVec1D			vhist;
		uVec1D			analyzed;
		uVec1D			minimized;
		uVec1D			shrinkable;
		LBDREST			lbdrest;
		LUBYREST		lubyrest;
		RANDOM			random;
		WALK			tracker;
		uint64			bumped;
		C_REF			conflict;
		C_REF			ignore;
		size_t			tablerowlen;
		string			tablerow;
		uint32			decisionlevel;
		int				decheuristic;
		bool			interrupted;
		bool			incremental;
		bool			stable;
		bool			probed;

	public:

		OPTION			opts;
		MODEL			model;
		PROOF			proof;
		CNF_ST			cnfstate;
		
						~Solver				() { }
						Solver				(const string& path);
		inline void		bumpShrunken		(CLAUSE& c);						
		inline void		bumpClause			(CLAUSE& c);
		inline int		calcLBD				(CLAUSE& c);
		inline int		removeRooted		(CLAUSE& c, const uint32* levels);
		inline void		removeSubsumed		(CLAUSE& c, const C_REF& cref, CLAUSE* subsuming);
		inline bool		subsumeCheck		(CLAUSE* subsuming, uint32& self, const LIT_ST* marks);
		inline CL_ST	subsumeClause		(CLAUSE& c, const cbucket_t* cs, const bool* deleted, const State_t* states, const C_REF& cref);
		inline void	    strengthenOTF		(CLAUSE& c, const C_REF& ref, const uint32& self);
		inline void		strengthen			(CLAUSE& c, const uint32& self);
		inline LIT_ST	sortClause			(CLAUSE& c, const int& start, const int& size, const bool& satonly);
		inline void		moveClause			(C_REF& r, CMM& newBlock, const cbucket_t* cs);
		inline void		moveWatches			(WL& ws, CMM& newBlock, const cbucket_t* cs);
		inline uint32	minReachable		(WL& ws, DFS* dfs, const DFS& node);
		inline bool		depFreeze			(const uint32& cand, const cbucket_t* cs, const LIT_ST* values, LIT_ST* frozen, uint32*& stack, WL& ws);
		inline void		MDMAssume			(const LIT_ST* values, const cbucket_t* cs, LIT_ST* frozen, uint32*& tail);
		inline bool		valid				(const LIT_ST* values, const cbucket_t* cs, WL& ws);
		inline void		recycleWL			(const uint32& lit, const cbucket_t* cs, const bool* deleted);
		inline bool		findBinary			(uint32 first, uint32 second, const cbucket_t* cs);
		inline bool		findTernary			(uint32 first, uint32 second, uint32 third, const cbucket_t* cs);
		inline void		minimizeBlock		(LEARNTLIT* bbegin, const LEARNTLIT* bend, const uint32& level, const uint32& uip);
		inline void		analyzeBlock		(LEARNTLIT* bbegin, const LEARNTLIT* bend, const uint32& level, const uint32& maxtrail);
		inline int		analyzeLit			(const uint32& level, const uint32& lit);
		inline void		analyzeLit			(const uint32& lit, int& track, int& size);
		inline uint32	analyzeReason		(const C_REF& ref, const uint32& lit);
		inline bool		analyzeReason		(const C_REF& ref, const uint32& parent, int& track);
		inline uint32	analyzeReason		(bool& failed, const uint32& level, const uint32& uip);
		inline bool		isBinary			(const C_REF& r, uint32& first, uint32& second);
		inline uint32	propAutarkClause	(const bool& add, const C_REF& ref, CLAUSE& c, const LIT_ST* values, LIT_ST* autarkies);
		inline bool		propProbe			(const uint32& assign);
		inline bool		propVivify			(const uint32& assign, const cbucket_t* cs, const bool* deleted);
		inline bool		propBinary			(const uint32& assign);
		inline void		cancelAssign		(const uint32& lit);
		inline void		cancelAutark		(const bool& add, const uint32& lit, LIT_ST* autarkies);
		inline bool		canDecompose		(const bool& first);
		inline void		pumpFrozenHeap		(const uint32& lit);
		inline void		pumpFrozenQue		(const uint32& lit);
		inline void		bumpReason			(const uint32& lit);
		inline void		bumpReasons			(const uint32& lit);
		inline void		bumpReasons			();
		inline void		bumpAnalyzed		();
		inline void		savePhases			();
		inline void		varOrgPhase			();
		inline void		varInvPhase			();
		inline void		varFlipPhase		();
		inline void		varBestPhase		();
		inline void		varWalkPhase		();
		inline void		varRandPhase		();
		inline uint32	where				();
		inline int		calcLBD				();
		inline bool		canVivify			();
		inline uint32	nextProbe			();
		inline void		nointerrupt			() { 
			interrupted = false;
		}
		inline void		interrupt			() { 
			interrupted = true;
		}	
		inline bool		canSimplify			() {
			if (!opts.simplify_en) return false;
			if (!ORIGINALS) return false;
			if (last.simplify.reduces > stats.reduces) return false;
			if (limit.simplify > stats.conflicts) return false;
			if (sp->simplified >= opts.simplify_min) return true;
			return ((stats.shrunken - last.shrink.removed) > (opts.simplify_min << 4));
		}
		inline bool		canMMD				() 
		{
			if (!opts.mdm_rounds) return false;
			assert(trail.size() <= inf.maxVar); 
			const bool enough = (inf.maxVar - trail.size()) > last.mdm.unassigned;
			const bool rounds = last.mdm.rounds;
			if (enough && !rounds && stats.conflicts >= limit.mdm) {
				last.mdm.rounds = opts.mdm_rounds;
				INCREASE_LIMIT(mdm, stats.mdm.calls, nlogn, true);
			}
			return enough && rounds;
		}
		inline bool		canRestart			()
		{
			if (!LEVEL) return false;
			if (stats.conflicts < limit.restart.conflicts) return false;
			vibrate();
			if (stable) return lubyrest;
			return lbdrest.restart(stable);
		}
		inline void		rootify				() {
			assert(UNSOLVED);
			assert(sp->propagated == trail.size());
			backtrack();
			if (BCP()) learnEmpty();
		}
		inline bool		retrail				() {
			assert(!LEVEL);
			sp->propagated = 0;
			if (BCP()) {
				learnEmpty();
				return true;
			}
			return false;
		}
		inline void		initQueue			() {
			LOGN2(2, " Initializing VMFQ Queue with %d variables..", inf.maxVar);
			vmtf.reserve(inf.maxVar);
			forall_variables(v) { 
				vmtf.insert(v);
				vmtf.update(v, (bumps[v] = ++bumped));
			}
			LOGDONE(2, 4);
		}
		inline void		initHeap			() {
			LOGN2(2, " Initializing variable heap(s) with %d variables..", inf.maxVar);
			vsidsheap.reserve(inf.maxVar);
			chbheap.reserve(inf.maxVar);
			forall_variables(v) {
				vsidsheap.insert(v);
				chbheap.insert(v);
			}
			LOGDONE(2, 5);
		}
		inline void		initVars			() {
			LOGN2(2, " Initializing original variables array with %d variables..", inf.maxVar);
			vorg.resize(inf.maxVar + 1);
			uint32* d = vorg;
			*d = 0;
			forall_variables(v) { d[v] = v; }
			LOGDONE(2, 5);
		}
		inline void		updateQueue			() {
			const uint32 last = vmtf.last();
			vmtf.update(last, bumps[last]);
		}
		inline void		updateHeap			() {
			dheap_t& heap = DECISIONHEAP;
			const State_t* states = sp->state;
			forall_variables(v) {
				if (!states[v].state && !heap.has(v))
					heap.insert(v);
			}
		}
		inline void		updateCHB			() {
			const State_t* states = sp->state;
			uint64* conflicts = chb.conflicts.data();
			forall_vector(uint32, analyzed, a) {
				if (!states[*a].state)
					conflicts[*a] = stats.conflicts;
			}
		}
		inline void		learnEmpty			() {
			if (opts.proof_en) 
				proof.addEmpty();
			SET_UNSAT;
		}
		inline void		clearLevels			() {
			forall_vector(uint32, lbdlevels, dl) {
				sp->state[*dl].dlcount = 0;
			}
			lbdlevels.clear();
		}
		inline void		clearMinimized		() {
			forall_vector(uint32, minimized, v) {
				sp->seen[*v] = 0;
			}
			minimized.clear();
		}
		inline void		clearAnalyzed		() {
			forall_vector(uint32, analyzed, a) {
				sp->seen[*a] = 0;
			}
			analyzed.clear();
		}
		inline void		clearVivify			() {
			forall_vector(uint32, analyzed, a) {
				CHECKLIT(*a);
				sp->seen[ABS(*a)] = 0;
			}
			analyzed.clear();
			learntC.clear();
		}
		inline void		clearFrozen			() {
			assert(sp->stacktail - sp->tmpstack <= inf.maxVar);
			LIT_ST* frozen = sp->frozen;
			for (uint32* i = sp->tmpstack, *end = sp->stacktail; end != i; ++i)
				frozen[*i] = 0;

			assert(_checkfrozen(sp->frozen, inf.maxVar));
		}
		inline void		clearMDM			() {
			assert(_checkMDM(sp->frozen, trail, sp->propagated));
			LIT_ST* seen = sp->seen;
			for (uint32* i = trail + sp->propagated, *end = trail.end(); i != end; ++i)
				seen[ABS(*i)] = 0;

			assert(_checkseen(sp->seen, inf.maxVar));
			assert((sp->stacktail - sp->tmpstack) <= (inf.maxVar - last.mdm.decisions));

			clearFrozen();
		}
		inline void		clearMapFrozen		() {
			if (*orgcore < UNDEF_VAR) {
				for (int i = 0; i < MAXFUNVAR; ++i) {
					assert(orgcore[i] <= inf.maxVar);
					sp->marks[orgcore[i]] = UNDEF_VAL;
				}
				*orgcore = UNDEF_VAR;
			}
		}
		inline void		markLit				(const uint32& lit) { CHECKLIT(lit); sp->marks[ABS(lit)] = SIGN(lit); }
		inline void		unmarkLit			(const uint32& lit) { CHECKLIT(lit); sp->marks[ABS(lit)] = UNDEF_VAL; }
		inline bool		active				(const uint32& lit) const { CHECKLIT(lit); return !sp->state[ABS(lit)].state; }
		inline bool		inactive			(const uint32& lit) const { CHECKLIT(lit); return sp->state[ABS(lit)].state; }
		inline bool		subsumed			(const uint32& lit) const { CHECKLIT(lit); return sp->marks[ABS(lit)] == SIGN(lit); }
		inline bool		notsubsumed			(const uint32& lit) const { CHECKLIT(lit); return sp->marks[ABS(lit)] ^ SIGN(lit); }
		inline bool		selfsubsumed		(const uint32& lit) const { CHECKLIT(lit); return sp->marks[ABS(lit)] == !SIGN(lit); }
		inline uint32	l2dl				(const uint32& lit) const { CHECKLIT(lit); return sp->level[ABS(lit)]; }
		inline C_REF	l2r					(const uint32& lit) const { CHECKLIT(lit); return sp->source[ABS(lit)]; }
		inline LIT_ST	l2marker			(const uint32& lit) const { CHECKLIT(lit); return sp->marks[ABS(lit)]; }
		inline LIT_ST	l2value				(const uint32& lit) const { CHECKLIT(lit); return sp->value[lit]; }
		inline LIT_ST	unassigned			(const uint32& lit) const { CHECKLIT(lit); return UNASSIGNED(sp->value[lit]); }
		inline bool		isFalse				(const uint32& lit) const { CHECKLIT(lit); return !sp->value[lit]; }
		inline bool		isTrue				(const uint32& lit) const 
		{ 
			CHECKLIT(lit);
			LIT_ST val = sp->value[lit];
			return val && !UNASSIGNED(val);
		}	
		inline void		enqueueDecision		(const uint32& lit) 
		{
			CHECKLIT(lit);
			assert(inf.unassigned);

			INCLEVEL(lit);
			enqueue(lit, LEVEL);
		}
		inline void		enqueueUnit			(const uint32& lit) 
		{
			CHECKLIT(lit);
			assert(active(lit));
			const uint32 v = ABS(lit);
			learnUnit(lit, v);
			sp->level[v] = 0;
			sp->value[lit] = 1;
			sp->value[FLIP(lit)] = 0;
			trail.push(lit);
			assert(inf.unassigned);
			inf.unassigned--;
#ifdef LOGGING
			LOGNEWLIT(3, UNDEF_REF, lit);
#endif
			if (wt.size()) {
				WL& ws = wt[lit];
				if (ws.size()) {
#if defined(_WIN32)
					PreFetchCacheLine(PF_TEMPORAL_LEVEL_1, &ws[0]);
#else
					__builtin_prefetch(&ws[0], 0, 1);
#endif
				}
			}
		}
		inline void		enqueue				(const uint32& lit, const uint32& level = 0, const C_REF src = UNDEF_REF) 
		{
			CHECKLIT(lit);
			CHECKLEVEL(level);
			assert(unassigned(lit));
			const uint32 v = ABS(lit);
			if (!probed) sp->phase[v].saved = SIGN(lit);
			sp->source[v] = src;
			sp->level[v] = level;
			sp->value[lit] = 1;
			sp->value[FLIP(lit)] = 0;
			sp->trailpos[v] = trail.size();
			trail.push(lit);
			assert(inf.unassigned);
			inf.unassigned--;
			if (!level) learnUnit(lit, v);
#ifdef LOGGING
			LOGNEWLIT(4, src, lit);
#endif
			assert(wt.size());
			WL& ws = wt[lit];
			if (ws.size()) {
#if defined(_WIN32)
				PreFetchCacheLine(PF_TEMPORAL_LEVEL_1, &ws[0]);
#else
				__builtin_prefetch(&ws[0], 0, 1);
#endif
			}
		}
		inline void		detachWatch			(const uint32& lit, const C_REF& ref) 
		{
			CHECKLIT(lit);
			assert(ref < UNDEF_REF);
			WL& ws = wt[lit];
			if (ws.empty()) return;
			WATCH *j = ws;
			forall_watches(ws, i) {
				const WATCH w = *i;
				if (NEQUAL(w.ref, ref))
					*j++ = w;
			}
			assert(j + 1 == ws.end());
			ws.resize(uint32(j - ws));
		}
		inline uint32	forcedLevel			(const uint32& lit, CLAUSE& c)
		{
			CHECKLIT(lit);
			uint32 flevel = 0;
			forall_clause(c, k) {
				const uint32 other = *k;
				if (NEQUAL(other, lit)) {
					assert(!unassigned(other));
					const uint32 otherlevel = l2dl(other);
					if (otherlevel > flevel) 
						flevel = otherlevel;
				}
			}
			CHECKLEVEL(flevel);
			return flevel;
		}
		inline void		learnUnit			(const uint32& lit, const uint32& v)
		{
			if (opts.proof_en) proof.addUnit(lit);
			assert(ABS(lit) == v);
			markFrozen(v);
		}		
		inline void		varBumpQueue		(const uint32& v) 
		{
			CHECKVAR(v);
			if (!vmtf.next(v)) return;
			vmtf.toFront(v);
			assert(bumped && bumped < UINT64_MAX);
			bumps[v] = ++bumped;
			if (UNASSIGNED(sp->value[V2L(v)])) vmtf.update(v, bumps[v]);
		}
		inline void		varBumpQueueNU		(const uint32& v) 
		{
			CHECKVAR(v);
			if (!vmtf.next(v)) return;
			assert(bumped && bumped < UINT64_MAX);
			vmtf.toFront(v);
			bumps[v] = ++bumped;
#ifdef LOGGING
			LOG2(4, " Variable %d moved to queue front & bumped to %lld", v, bumped);
#endif
		}
		inline void		markFrozen			(const uint32& v)
		{
			CHECKVAR(v);
			sp->state[v].state = FROZEN_M;
			inf.maxFrozen++;
		}
		inline void		markEliminated		(const uint32& v) 
		{
			CHECKVAR(v);
			assert(!sp->state[v].state);
			sp->state[v].state = MELTED_M;
			assert(inf.unassigned);
			inf.unassigned--;
		}
		inline void		markSubstituted		(const uint32& v) 
		{
			CHECKVAR(v);
			assert(!sp->state[v].state);
			sp->state[v].state = SUBSTITUTED_M;
			assert(inf.unassigned);
			inf.unassigned--;
			inf.maxSubstituted++;
		}
		void			shrinkClause		(const C_REF& r, const uint32* levels, const LIT_ST* values);
		void			shrinkClause		(CLAUSE& c, const int& remLits);
		void			removeClause		(CLAUSE& c, const C_REF& cref);
		void			sortClause			(CLAUSE& c);
		bool			hyper3Resolve		(CLAUSE& pos, CLAUSE& neg, const uint32& p);
		uint32			hyper2Resolve		(uint32* lits, const int csize, const uint32& lit);
		bool			vivifyClause		(const C_REF& cref);
		bool			vivifyAnalyze		(CLAUSE& cand, bool& original);
		bool			vivifyLearn			(CLAUSE& cand, const C_REF& cref, const int& nonFalse, const bool& original);
		void			addClause			(SCLAUSE& src);
		C_REF			addClause			(const Lits_t& src, const bool& learnt);
		void			addClause			(const C_REF& cref, CLAUSE& c, const bool& learnt);
		bool			makeClause			(Lits_t& c, Lits_t& org, char*& str);
		void			backtrack			(const uint32& jmplevel = 0);
		void			map					(const bool& sigmified = false);
		void			recycle				(CMM& new_cm);
		void			recycleWT			(const cbucket_t* cs, const bool* deleted);
		void			filter				(BCNF& cnf, const bool* deleted);
		void			filter				(BCNF& cnf, CMM& new_cm, const cbucket_t* cs, const bool* deleted);
		void			histBins			(BCNF& cnf);
		void			shrink				(BCNF& cnf);
		void			shrinkTop			(BCNF& cnf);
		void			sortVivify			(BCNF& cnf);
		void			scheduleForward		(BCNF& cnf, const cbucket_t* cs, const bool* deleted);
		void			scheduleVivify		(BCNF& cnf, const bool& tier2, const bool& learnt);
		void			histCNF				(BCNF& cnf, const bool& reset = false);
		void			attachBins			(BCNF& cnf, const cbucket_t* cs, const bool* deleted, const bool& hasElim = false);
		void			attachNonBins		(BCNF& cnf, const cbucket_t* cs, const bool* deleted, const bool& hasElim = false);
		void			attachClauses		(BCNF& cnf, const cbucket_t* cs, const bool* deleted, const bool& hasElim = false);
		bool			substitute			(BCNF& cnf, uint32* smallests);
		void			attachTernary		(BCNF& cnf, LIT_ST* use, const cbucket_t* cs, const bool* deleted);
		void			scheduleTernary		(LIT_ST* use);
		uint32			autarkReasoning		(LIT_ST* autarkies);
		uint32			useAutarky			(LIT_ST* autarkies);
		uint32			propAutarky			(const LIT_ST* values, LIT_ST* autarkies);
		void			vivifying			(const CL_ST& type);
		void			ternarying			(const uint64& resolvents_limit, const uint64& checks_limit);
		void			ternaryResolve		(const uint32& p, const uint64& limit);
		void			subsumeLearnt		(const C_REF& lref);
		uint32			makeAssign			(const uint32& v, const bool& tphase = false);
		bool			minimize			(const uint32& lit, const int& depth = 0);
		void			rebuildWT			(const CL_ST& priorbins = 0);
		void			binarizeWT			(const bool& keeplearnts);
		void			detachClauses		(const bool& keepbinaries);
		void			decompose			(const bool& first);
		void			shrinkTop			(const bool& conditional);
		void			newHyper3			(const bool& learnt);
		uint32			nextHeap			(dheap_t& heap);
		uint32			nextQueue			();
		void			newHyper2			();
		bool			shrink				();
		bool			decompose			();
		void			debinary			();
		void			ternary				();
		void			transitive			();
		void			vivify				();
		void			sortWT				();
		void			pumpFrozen			();
		void			allocSolver			();
		void			initLimits			();
		void			initSolver			();
		void			killSolver			();
		void			markReasons		    ();
		void			unmarkReasons	    ();
		void			recycle				();
		void			reduce				();
		void			reduceLearnts		();
		void			rephase				();
		void			autarky				();
		void			filterAutarky		();
		void			forward				();
		void			forwardAll			();
		void			filterOrg			();
		void			minimize			();
		void			minimizebin			();
		void			minimizeall			();
		void			vibrate				();
		void			updateModeLimit		();
		void			updateUnstableLimit	();
		void			stableMode			();
		void			unstableMode		();
		void			restart				();
		void			probe				();
		void			failing				();
		void			scheduleProbes		();
		uint32			reuse				();
		C_REF			learn				();
		void			analyze				();
		bool			finduip				();
		bool			chronoAnalyze		();
		bool			chronoHasRoot		();
		bool			BCPVivify			();
		bool			BCPProbe			();
		bool			BCP					();
		void			MDMInit				();
		void			MDM					();
		void			decide				();
		void			report				();
		void			wrapup				();
		bool			parser				();
		void			solve				();

		//==========================================//
		//                Simplifier                //
		//==========================================//
	protected:
		uVec1D	elected;
		SCNF	scnf;
		OT		ot;
		uint32	multiplier;
		int		phase;
		int		numforced;
		int		simpstate;
		bool	mapped;

	public:

		inline void		resizeCNF			() {
			int times = phase + 1;
			if (times > 1 && times != opts.phases && (times % opts.collect_freq) == 0)
				shrinkSimp();
		}
		inline void		initSimp			() {
			phase = multiplier = numforced = 0;
			simpstate = AWAKEN_SUCC;
		}
		inline void		filterElected		() {
			uint32* i = elected, *j = i; 
			uint32* end = elected.end();
			while (i != end) {
				const uint32 x = *i++;
				if (x && !sp->state[x].state) 
					*j++ = x;
			}
			elected.resize(uint32(j - elected));
		}
		inline void		countMelted			() {
			inf.nDeletedVarsAfter = 0;
			forall_variables(v) {
				if (MELTED(sp->state[v].state))
					inf.nDeletedVarsAfter++;
			}
			assert(inf.nDeletedVarsAfter >= inf.maxMelted);
			inf.nDeletedVarsAfter -= inf.maxMelted;
			inf.maxMelted += inf.nDeletedVarsAfter;
		}
		inline void		countFinal			() {
			countMelted();
			countAll();
			inf.nClauses = inf.nClausesAfter;
			inf.nLiterals = inf.nLiteralsAfter;	
		}
		inline void		countAll			() {
			inf.nClausesAfter = 0;
			inf.nLiteralsAfter = 0;
			forall_sclauses(scnf, i) {
				const SCLAUSE& s = scnf[*i];
				if (s.original() || s.learnt()) {
					inf.nClausesAfter++;
					inf.nLiteralsAfter += s.size();
				}
			}
		}
		inline void		countCls			() {
			inf.nClausesAfter = 0;
			forall_sclauses(scnf, i) {
				const SCLAUSE& s = scnf[*i];
				if (s.original() || s.learnt())
					inf.nClausesAfter++;
			}
		}
		inline void		countLits			() {
			inf.nLiteralsAfter = 0;
			forall_sclauses(scnf, i) {
				const SCLAUSE& s = scnf[*i];
				if (s.original() || s.learnt())
					inf.nLiteralsAfter += s.size();
			}
		}
		inline void		evalReds			() {
			inf.nClausesAfter = 0;
			inf.nLiteralsAfter = 0;
			forall_sclauses(scnf, i) {
				const SCLAUSE& s = scnf[*i];
				if (s.original() || s.learnt()) {
					inf.nClausesAfter++;
					inf.nLiteralsAfter += s.size();
				}
			}
			countMelted();
		}
		inline void		logReductions		() {
			int64 varsRemoved	= int64(inf.nDeletedVarsAfter) + numforced;
			int64 clsRemoved	= int64(inf.nClauses)	- inf.nClausesAfter;
			int64 litsRemoved	= int64(inf.nLiterals)	- inf.nLiteralsAfter;
			const char *header	= "  %s%-10s  %-10s %-10s %-10s%s";
			LOG1(header, CREPORT, " ", "Variables", "Clauses", "Literals", CNORMAL);
			const char* rem = "  %s%-10s: %s%-9lld  %c%-8lld  %c%-8lld%s";
			const char* sur = "  %s%-10s: %s%-9d  %-9d  %-9d%s";
			LOG1(rem, CREPORT, "Removed", CREPORTVAL, 
				-varsRemoved,
				clsRemoved < 0 ? '+' : '-', abs(clsRemoved), 
				litsRemoved < 0 ? '+' : '-', abs(litsRemoved), CNORMAL);
			LOG1(sur, CREPORT, "Survived", CREPORTVAL,
				ACTIVEVARS,
				inf.nClausesAfter,
				inf.nLiteralsAfter, CNORMAL);
		}
		inline void		removeClause		(SCLAUSE& c)
		{
			if (!c.deleted()) {
				c.markDeleted();
				if (opts.proof_en)
					proof.deleteClause(c);
			}
		}
		inline void		bumpShrunken		(SCLAUSE& c)
		{
			assert(c.learnt());
			assert(c.size() > 1);
			const int old_lbd = c.lbd();
			if (old_lbd <= opts.lbd_tier1) return; // always keep Tier1 value
			const int size = c.size() - 1;
			int new_lbd = MIN(size, old_lbd);
			if (new_lbd >= old_lbd) return;
			c.set_lbd(new_lbd);
			c.set_usage(USAGET3);
			LOGCLAUSE(4, c, " Bumping shrunken clause with (lbd:%d, usage:%d) ", new_lbd, c.usage());
		}
		inline void		addResolvent		(const Lits_t& resolvent, const S_REF& ref);
		inline void		resolve				(const uint32& x, Lits_t& out_c, const int& nAddedCls, const int& nAddedLits);
		inline void		resolveCore			(const uint32& x, Lits_t& out_c, const int& nAddedCls, const int& nAddedLits);
		inline void		substitute			(const uint32& x, Lits_t& out_c, const int& nAddedCls, const int& nAddedLits);
		inline bool		propClause			(const LIT_ST* values, const uint32& lit, SCLAUSE& c);
		inline bool		depFreeze			(OL& ol, OCCUR* occs, LIT_ST* frozen, uint32*& tail, const uint32& cand, const uint32& pmax, const uint32& nmax);
		inline bool		checkMem			(const string& name, const size_t& size);
		void			strengthen			(SCLAUSE& c, const uint32& me);
		void			extract				(BCNF& cnf);
		void			histCNF				(SCNF& cnf, const bool& reset = false);
		void			reduceOL			(OL& ol);
		void			createOT			(const bool& reset = true);
		void			newBeginning		();
		void			shrinkSimp			();
		void			simplifying			();
		void			simplify			();
		void			awaken				();
		bool			LCVE				();
		bool			prop				();
		void			VE					();
		void			SUB					();
		void			ERE					();
		void			BCE					();
		void			sortOT				();
		void			reduceOT			();

		//==========================================//
		//             Local search                 //
		//==========================================//
		inline bool		popUnsat			(const uint32& infoidx, const uint32& unsatidx, Vec<CINFO>& cinfo);
		inline void		saveTrail			(const LIT_ST* values, const bool& keep);
		inline void		saveAll				(const LIT_ST* values);
		inline uint32	breakValue			(const uint32& lit);
		inline void		makeClauses			(const uint32& lit);
		inline void		breakClauses		(const uint32& lit);
		inline void		walkassign			();
		uint32			promoteLit			();
		uint32			ipromoteLit			();
		void			updateBest			();
		bool			walkschedule		();
		void			walkinit			();
		void			walkstep			();
		void			walkstop			();
		void			walking				();
		void			walk				();

		//==========================================//
		//          Incremental Solving             //
		//==========================================//
	protected:

		Vec<LIT_ST>		ifrozen;
		Vec<LIT_ST>		ivalue;
		Vec<LIT_ST>		imarks;
		Vec<State_t>	ivstate;
		Lits_t			iconflict;
		uVec1D			ilevel;
		uVec1D			assumptions;

	public:
						Solver				();
		void			iunassume			();
		void			iallocSpace			();
		uint32			iadd			    ();
		void			idecide				();
		void			ianalyze			(const uint32& failed);
		bool			itoClause			(Lits_t& c, Lits_t& org);
		void			iassume				(Lits_t& assumptions);
		void			isolve				(Lits_t& assumptions);
		bool		    ifailed             (const uint32& v);
		void		    ifreeze             (const uint32& v);
		void		    iunfreeze           (const uint32& v);
		bool		    ieliminated         (const uint32& v);
		inline uint32   imap                (const uint32& v) const {
			assert(v && v < UNDEF_VAR);
			assert(model.lits.size() > v);
			return model.lits[v];
		}
		inline bool		iassumed            (const uint32& v) const {
			CHECKVAR(v);
			return incremental && ifrozen[v];
		}

		//==========================================//
		//			       Printers                 //
		//==========================================//
		void			printStats			(const bool& printing = true, const Byte& type = ' ', const char* color = CNORMAL);
		void			printVars			(const uint32* arr, const uint32& size, const LIT_ST& type = 'x');
		void			printClause			(const Lits_t& c);
		void			printTrail			(const uint32& off = 0);
		void			printCNF			(const BCNF& cnf, const int& off = 0);
		void			printOL				(const OL& list);
		void			printOL				(const uint32& lit);
		void			printOccurs			(const uint32& v);
		void			printWL				(const uint32& lit, const bool& bin = 0);
		void			printWL				(const WL& ws, const bool& bin = 0);
		void			printWatched		(const uint32& v);
		void			printBinaries		(const uint32& v);
		void			printSortedStack	(const int& tail);
		void			printTable			();
		void			printWT				();
		void			printOT				();
		void			printHeap			();
		void			printSource			();
		void			printLearnt			();
		
	};

	extern Solver* solver;

}

#endif 
