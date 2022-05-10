/***********************************************************************[options.cpp]
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

#include "options.hpp"

using namespace SeqFROST;

// simplifier options
BOOL_OPT opt_all_en("all", "enable all simplifications", false);
BOOL_OPT opt_aggr_cnf_sort("aggresivesort", "sort simplified formula with aggresive key before writing to host", false);
BOOL_OPT opt_bce_en("blocked", "enable blocked clause elimination (BCE)", false);
BOOL_OPT opt_sub_en("subsume", "enable subsumption elimination within simplifications (SUB)", true);
BOOL_OPT opt_ere_en("redundancy", "enable eager redundancy elimination (ERE)", true);
BOOL_OPT opt_solve_en("solve", "proceed with solving after simplifications", true);
BOOL_OPT opt_profile_simp_en("profilesimplifier", "profile simplifications", false);
BOOL_OPT opt_ve_en("bounded", "enable bounded variable elimination (BVE)", true);
BOOL_OPT opt_ve_lbound_en("bvebound", "skip variables resulting in more literals than original", false);
BOOL_OPT opt_ve_fun_en("function", "enable function table reasoning", true);
BOOL_OPT opt_ve_plus_en("boundedextend", "enable subsumption elimination before variable elimination", true);

INT_OPT opt_lcve_min_vars("electionsmin", "minimum elected variables to simplify", 1, INT32R(1, INT32_MAX));
INT_OPT opt_lcve_max_occurs("electionsmax", "maximum occurrence list size to check in elections (LCVE)", 3e3, INT32R(1, INT32_MAX));
INT_OPT opt_lcve_clause_max("electionsclausemax", "maximum clause size to check in LCVE", 3e4, INT32R(1, INT32_MAX));
INT_OPT opt_bce_max_occurs("blockedmaxoccurs", "maximum occurrence list size to scan in BCE", 1e3, INT32R(100, INT32_MAX));
INT_OPT opt_collect_freq("collectfrequency", "set the frequency of CNF memory shrinkage in the simplifier", 2, INT32R(0, 5));
INT_OPT opt_ere_extend("redundancyextend", "extend ERE with clause strengthening (0: no extend, 1: originals, 2: all)", 1, INT32R(0, 3));
INT_OPT opt_ere_max_occurs("redundancymaxoccurs", "maximum occurrence list size to scan in ERE", 1e3, INT32R(100, INT32_MAX));
INT_OPT opt_ere_clause_max("redundancyclausemax", "maximum resolvent size for forward check in ERE", 200, INT32R(2, INT32_MAX));
INT_OPT opt_sub_max_occurs("subsumemaxoccurs", "maximum occurrence list size to scan in SUB", 1e3, INT32R(100, INT32_MAX));
INT_OPT opt_sub_clause_max("subsumeclausemax", "maximum clause size to check in SUB", 100, INT32R(0, INT32_MAX));
INT_OPT opt_mu_pos("mupos", "set the positive occurrences limit in LCVE", 32, INT32R(10, INT32_MAX));
INT_OPT opt_mu_neg("muneg", "set the negative occurrences limit in LCVE", 32, INT32R(10, INT32_MAX));
INT_OPT opt_phases("phases", "set the number of phases in to run simplifications", 5, INT32R(0, INT32_MAX));
INT_OPT opt_lits_phase_min("eliminationphasemin", "minimum removed literals per phase to stop simplifications", 500, INT32R(1, INT32_MAX));
INT_OPT opt_ve_clause_max("boundedclausemax", "maximum resolvent size (0: no limit)", 100, INT32R(0, INT32_MAX));
INT_OPT opt_xor_max_arity("xormaxarity", "maximum XOR fanin size", 10, INT32R(2, 20));

// solver options
BOOL_OPT opt_autarky_en("autarky", "enable autarky reasoning as in Look-ahead solvers", true);
BOOL_OPT opt_autarky_sleep_en("autarkysleep", "allow autarky reasoning to sleep", true);
BOOL_OPT opt_boundsearch_en("boundsearch", "activate search bounds on decisions and/or conflicts", false);
BOOL_OPT opt_chrono_en("chrono", "enable chronological backtracking", true);
BOOL_OPT opt_bumpreason_en("bumpreason", "bump reason literals via learnt clause", true);
BOOL_OPT opt_debinary_en("debinary", "remove duplicated binaries", true);
BOOL_OPT opt_decompose_en("decompose", "decompose binary implication gragh into SCCs", true);
BOOL_OPT opt_time_quiet_en("timequiet", "report time even in quiet mode", false);
BOOL_OPT opt_targetonly_en("targetonly", "use only target phase", false);
BOOL_OPT opt_ternary_en("ternary", "enable hyper ternary resolution", true);
BOOL_OPT opt_ternary_sleep_en("ternarysleep", "allow hyper ternary resolution to sleep", true);
BOOL_OPT opt_transitive_en("transitive", "enable transitive reduction on binary implication graph", true);
BOOL_OPT opt_parseonly_en("parseonly", "parse only the input formula", false);
BOOL_OPT opt_parseincr_en("parseincr", "parse input formula incrementally", false);
BOOL_OPT opt_polarity("polarity", "initial variable polarity", true);
BOOL_OPT opt_proof_en("proof", "enable DRAT proof generation (default: binary)", false);
BOOL_OPT opt_proof_nonbinary_en("proofnonbinary", "generate proof in binary DRAT format", false);
BOOL_OPT opt_probe_en("probe", "enable failed literal probing", true);
BOOL_OPT opt_probe_sleep_en("probesleep", "allow failed literal probing to sleep", true);
BOOL_OPT opt_probehbr_en("probehyper", "learn hyper binary clauses", true);
BOOL_OPT opt_model_en("model", "extend model with eliminated variables", false);
BOOL_OPT opt_modelprint_en("modelprint", "print model on stdout", false);
BOOL_OPT opt_modelverify_en("modelverify", "verify model on input formula", false);
BOOL_OPT opt_mdmlcv_en("mdmlcv", "use least-constrained variables to make multiple decisions", false);
BOOL_OPT opt_mdmwalkinit_en("mdmwalkinit", "enable walk within an initial mdm round before search", true);
BOOL_OPT opt_mdmwalk_en("mdmwalk", "enable walk within an initial mdm round", false);
BOOL_OPT opt_mdmvsidsonly_en("mdmvsidsonly", "enable VSIDS only in MDM (VMFQ disabled)", false);
BOOL_OPT opt_mdmassume_en("mdmassume", "choose multiple decisions based on given assumptions (incremental mode)", false);
BOOL_OPT opt_minimize_en("minimize", "minimize learnt clause", true);
BOOL_OPT opt_minimizebin_en("minimizebin", "minimize learnt clause using binaries on the implication graph", true);
BOOL_OPT opt_minimizeall_en("minimizeall", "minimize further using all-UIP reasoning", false);
BOOL_OPT opt_minimizesort_en("minimizesort", "sort learnt clause before minimization", false);
BOOL_OPT opt_report_en("report", "allow performance report on stdout", true);
BOOL_OPT opt_rephase_en("rephase", "enable variable rephasing", true);
BOOL_OPT opt_reduce_en("reduce", "enable learnt database reduction", true);
BOOL_OPT opt_preprocess_en("preprocess", "enable preprocessing using CPU-like SIGmA", true);
BOOL_OPT opt_simplify_en("simplify", "enable live simplifications using CPU-like SIGmA", true);
BOOL_OPT opt_simplify_sleep_en("simplifysleep", "allow live simplifications to sleep", true);
BOOL_OPT opt_forward_en("forward", "enable forward subsumption elimination", true);
BOOL_OPT opt_vivify_en("vivify", "enable vivification", true);

INT_OPT opt_chrono_min("chronomin", "minimum distance to trigger chronological backtracking", 100, INT32R(0, INT32_MAX));
INT_OPT	opt_decheuristic("decisionheuristic", "initial decision heap heuristic (0: VSIDS, 1: CHB)", 0, INT32R(0, 2));
INT_OPT opt_decompose_min("decomposemin", "minimum rounds to decompose", 2, INT32R(1, 10));
INT_OPT opt_decompose_limit("decomposelimit", "decompose round limit", 1e7, INT32R(0, 10));
INT_OPT opt_decompose_min_eff("decomposemineff", "decompose minimum efficiency", 1e7, INT32R(0, INT32_MAX));
INT_OPT opt_mdm_heappumps("mdmheappumps", "set the number of follow-up decision pumps using Heap score", 0, INT32R(0, 3));
INT_OPT opt_mdm_vmtfpumps("mdmvmtfpumps", "set the number of follow-up decision pumps using VMFQ score", 1, INT32R(0, 3));
INT_OPT opt_mdm_maxoccurs("mdmmaxoccurs", "maximum occurrence list size in MDM", 3e3, INT32R(1, INT32_MAX));
INT_OPT opt_mdm_rounds("mdmrounds", "set the number of mdm rounds in a single search", 3, INT32R(0, 10));
INT_OPT opt_mdm_inc("mdminc", "MDM increment value based on conflicts", 2e3, INT32R(0, INT32_MAX));
INT_OPT opt_minimize_lbd("minimizelbd", "minimum LBD to do binary strengthening", 6, INT32R(1, 100));
INT_OPT opt_minimize_min("minimizemin", "minimum learnt size to do binary strengthening", 30, INT32R(1, 1000));
INT_OPT opt_minimize_depth("minimizedepth", "minimization depth to explore", 1e3, INT32R(1, INT32_MAX));
INT_OPT opt_mode_inc("modeinc", "mode increment value based on conflicts", 1e3, INT32R(1, INT32_MAX));
INT_OPT opt_nap("nap", "maximum naping period", 2, INT32R(0, 10));
INT_OPT opt_ternary_priorbins("ternarypriorbins", "prioritize binaries in watch table after hyper ternary resolution (1: enable, 2: prioritize learnts)", 0, INT32R(0, 2));
INT_OPT opt_ternary_max_eff("ternarymaxeff", "maximum hyper ternary resolution efficiency", 1e2, INT32R(0, INT32_MAX));
INT_OPT opt_ternary_min_eff("ternarymineff", "minimum hyper ternary resolution efficiency", 1e6, INT32R(0, INT32_MAX));
INT_OPT opt_ternary_rel_eff("ternaryreleff", "relative hyper ternary resolution efficiency per mille", 40, INT32R(0, 1000));
INT_OPT opt_transitive_max_eff("transitivemaxeff", "maximum transitive efficiency", 1e2, INT32R(0, INT32_MAX));
INT_OPT opt_transitive_min_eff("transitivemineff", "minimum transitive efficiency", 1e6, INT32R(0, INT32_MAX));
INT_OPT opt_transitive_rel_eff("transitivereleff", "relative transitive efficiency per mille", 20, INT32R(0, 1000));
INT_OPT opt_simplify_inc("simplifyinc", "simplifying increment value based on conflicts", 500, INT32R(1, INT32_MAX));
INT_OPT opt_simplify_min("simplifymin", "minimum root variables shrunken to awaken live simplifications", 4e3, INT32R(1, INT32_MAX));
INT_OPT opt_simplify_priorbins("simplifypriorbins", "prioritize binaries in watch table after sigmification (1: enable, 2: prioritize learnts)", 1, INT32R(0, 2));
INT_OPT opt_restart_inc("restartinc", "restart increment value based on conflicts", 1, INT32R(1, INT32_MAX));
INT_OPT opt_reduce_inc("reduceinc", "increment value of clauses reduction based on conflicts", 300, INT32R(10, INT32_MAX));
INT_OPT opt_rephase_inc("rephaseinc", "rephasing increment value based on conflicts", 600, INT32R(100, INT32_MAX));
INT_OPT opt_progress("progressrate", "progress rate to print search statistics", 1e4, INT32R(1, INT32_MAX));
INT_OPT opt_probe_inc("probeinc", "probe increment value based on conflicts", 100, INT32R(1, INT32_MAX));
INT_OPT opt_probe_min("probemin", "minimum rounds to probe", 2, INT32R(1, 10));
INT_OPT opt_probe_max_eff("probemaxeff", "maximum probe efficiency", 1e2, INT32R(0, INT32_MAX));
INT_OPT opt_probe_min_eff("probemineff", "minimum probe efficiency", 5e5, INT32R(0, INT32_MAX));
INT_OPT opt_probe_rel_eff("probereleff", "relative probe efficiency per mille", 2, INT32R(0, 1000));
INT_OPT opt_stable("stable", "enable variable phases stabilization based on restarts (0: UNSAT, 1: neutral, 2: SAT)", 1, INT32R(0, 2));
INT_OPT opt_seed("seed", "initial seed value for the random generator", 1008001, INT32R(1, INT32_MAX));
INT_OPT opt_forward_priorbins("forwardpriorbins", "prioritize binaries in watch table after forward (1: enable, 2: prioritize learnts)", 1, INT32R(0, 2));
INT_OPT opt_forward_inc("forwardinc", "forward subsumption increment value based on conflicts", 2e3, INT32R(100, INT32_MAX));
INT_OPT opt_forward_max_occs("forwardmaxoccurs", "maximum occurrences to subsume or strengthen in forward subsumption", 1e3, INT32R(10, INT32_MAX));
INT_OPT opt_forward_max_csize("forwardmaxcsize", "maximum forward subsuming clause size", 1e3, INT32R(2, INT32_MAX));
INT_OPT opt_forward_max_eff("forwardmaxeff", "maximum number of clauses to scan in forward subsumption", 1e2, INT32R(0, INT32_MAX));
INT_OPT opt_forward_min_eff("forwardmineff", "minimum number of clauses to scan in forward subsumption", 1e6, INT32R(0, INT32_MAX));
INT_OPT opt_forward_rel_eff("forwardreleff", "relative forward subsumption efficiency per mille", 1e4, INT32R(0, INT32_MAX));
INT_OPT opt_vivify_priorbins("vivifypriorbins", "prioritize binaries in watch table before vivification (1: enable, 2: prioritize learnts)", 0, INT32R(0, 2));
INT_OPT opt_vivify_max_eff("vivifymaxeff", "maximum vivify efficiency", 50, INT32R(0, INT32_MAX));
INT_OPT opt_vivify_min_eff("vivifymineff", "minimum vivify efficiency", 2e5, INT32R(0, INT32_MAX));
INT_OPT opt_vivify_rel_eff("vivifyreleff", "relative vivify efficiency per mille", 2, INT32R(0, 1000));
INT_OPT opt_walk_priorbins("walkpriorbins", "prioritize binaries in watch table after walking (1: enable, 2: prioritize learnts)", 1, INT32R(0, 2));
INT_OPT opt_walk_max_eff("walkmaxeff", "maximum Walksat efficiency", 1e2, INT32R(0, INT32_MAX));
INT_OPT opt_walk_min_eff("walkmineff", "minimum Walksat efficiency", 1e7, INT32R(0, INT32_MAX));
INT_OPT opt_walk_rel_eff("walkreleff", "relative Walksat efficiency per mille", 10, INT32R(0, 1000));
INT_OPT opt_lbd_tier1("lbdtier1", "lbd value of tier 1 learnts", 2, INT32R(1, INT32_MAX));
INT_OPT opt_lbd_tier2("lbdtier2", "lbd value of tier 2 learnts", 6, INT32R(3, INT32_MAX));
INT_OPT opt_lbd_fast("lbdfast", "initial lbd fast window", 33, INT32R(1, 100));
INT_OPT opt_lbd_slow("lbdslow", "initial lbd slow window", 1e5, INT32R(100, INT32_MAX));
INT_OPT opt_luby_inc("lubyinc", "luby increment value based on conflicts", 1 << 10, INT32R(1, INT32_MAX));
INT_OPT opt_luby_max("lubymax", "luby sequence maximum value", 1 << 20, INT32R(1, INT32_MAX));
INT_OPT opt_learntsub_max("subsumelearntmax", "maximum learnt clauses to subsume", 20, INT32R(0, INT32_MAX));
INT64_OPT opt_conflictout("conflictout", "set out-of-conflicts limit (must be enabled by \"boundsearch\")", INT64_MAX, INT64R(0, INT64_MAX));
INT64_OPT opt_decisionout("decisionout", "set out-of-decisions limit (must be enabled by \"boundsearch\")", INT64_MAX, INT64R(0, INT64_MAX));
DOUBLE_OPT opt_chb_step("chbstep", "CHB step", 0.4, FP64R(0, 1));
DOUBLE_OPT opt_chb_step_min("chbstepmin", "CHB step min", 0.06, FP64R(0, 1));
DOUBLE_OPT opt_chb_step_decay("chbstepdecay", "CHB step decay", 0.000001, FP64R(0, 1));
DOUBLE_OPT opt_stable_rate("stablerestartrate", "stable restart increase rate", 1.0, FP64R(1, 5));
DOUBLE_OPT opt_lbd_rate("lbdrate", "slow rate in firing lbd restarts", 1.1, FP64R(1, 10));
DOUBLE_OPT opt_ternary_perc("ternaryperc", "percentage of maximum hyper clauses to add", 0.2, FP64R(0, 1));
DOUBLE_OPT opt_mab_constant("mabconstant", "MAB constant", 4.0, FP64R(0, 10));
DOUBLE_OPT opt_map_perc("mapperc", "minimum percentage of variables to map", 0.2, FP64R(0, 1));
DOUBLE_OPT opt_reduce_perc("reduceperc", "percentage of learnt clauses to reduce", 0.75, FP64R(0.1, 1));
DOUBLE_OPT opt_var_inc("varinc", "VSIDS increment value", 1.0, FP64R(1, 10));
DOUBLE_OPT opt_var_decay("vardecay", "VSIDS decay value", 0.95, FP64R(0, 1));
DOUBLE_OPT opt_garbage_perc("collect", "collect garbage if its percentage exceeds this value", 0.25, FP64R(0, 1));
STRING_OPT opt_proof_out("proofout", "output file to write binary proof", "proof.out");

#if defined(__linux__) || defined(__CYGWIN__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnonnull-compare"
#endif

OPTION::OPTION() 
{
	RESETSTRUCT(this);
	int MAXLEN = 256;
	proof_path = sfcalloc<char>(MAXLEN);
}

#if defined(__linux__) || defined(__CYGWIN__)
#pragma GCC diagnostic pop
#endif

OPTION::~OPTION() 
{
	if (proof_path != NULL) {
		std::free(proof_path);
		proof_path = NULL;
	}
}

void OPTION::init() 
{
	autarky_en			= opt_autarky_en;
	autarky_sleep_en	= opt_autarky_sleep_en;
	bumpreason_en		= opt_bumpreason_en;
	chrono_en			= opt_chrono_en;
	chrono_min			= opt_chrono_min;
	chb_step			= opt_chb_step;
	chb_step_decay		= opt_chb_step_decay;
	chb_step_min		= opt_chb_step_min;
	conflict_out		= opt_conflictout;
	decision_out		= opt_decisionout;
	decheuristic		= opt_decheuristic;
	debinary_en			= opt_debinary_en;
	decompose_en		= opt_decompose_en;
	decompose_min		= opt_decompose_min;
	decompose_limit		= opt_decompose_limit;
	decompose_min_eff	= opt_decompose_min_eff;
	mab_constant		= opt_mab_constant;
	model_en			= opt_model_en;
	modelprint_en		= opt_modelprint_en;
	modelverify_en		= opt_modelverify_en;
	mode_inc			= opt_mode_inc;
	minimize_en			= opt_minimize_en;
	minimizebin_en		= opt_minimizebin_en;
	minimizeall_en		= opt_minimizeall_en;
	minimizesort_en		= opt_minimizesort_en;
	minimize_min		= opt_minimize_min;
	minimize_lbd		= opt_minimize_lbd;
	minimize_depth		= opt_minimize_depth;
	mdmvsidsonly_en		= opt_mdmvsidsonly_en;
	mdmassume_en		= opt_mdmassume_en;
	mdm_mcv_en			= !opt_mdmlcv_en;
	mdm_walk_init_en	= opt_mdmwalkinit_en;
	mdm_walk_en			= opt_mdmwalk_en;
	mdm_heap_pumps		= opt_mdm_heappumps;
	mdm_vmtf_pumps		= opt_mdm_vmtfpumps;
	mdm_max_occs        = opt_mdm_maxoccurs;
	mdm_rounds			= opt_mdm_rounds;
	mdm_inc				= opt_mdm_inc;
	map_perc			= opt_map_perc;
	nap					= opt_nap;
	memcpy(proof_path, opt_proof_out, opt_proof_out.length());
	parseonly_en		= opt_parseonly_en;
	parseincr_en		= opt_parseincr_en;
	proof_en			= opt_proof_en;
	proof_nonbinary_en	= opt_proof_nonbinary_en;
	probe_en			= opt_probe_en;
	probe_sleep_en		= opt_probe_sleep_en;
	probehbr_en			= opt_probehbr_en;
	probe_inc			= opt_probe_inc;
	probe_min			= opt_probe_min;
	probe_min_eff		= opt_probe_min_eff;
	probe_max_eff		= opt_probe_max_eff;
	probe_rel_eff		= opt_probe_rel_eff;
	prograte			= opt_progress;
	polarity			= opt_polarity;
	time_quiet_en		= opt_time_quiet_en;
	targetonly_en		= opt_targetonly_en;
	ternary_en			= opt_ternary_en;
	ternary_sleep_en	= opt_ternary_sleep_en;
	ternary_priorbins	= opt_ternary_priorbins;
	ternary_min_eff		= opt_ternary_min_eff;
	ternary_max_eff		= opt_ternary_max_eff;
	ternary_rel_eff		= opt_ternary_rel_eff;
	ternary_perc		= opt_ternary_perc;
	transitive_en		= opt_transitive_en;
	transitive_min_eff  = opt_transitive_min_eff;
	transitive_max_eff  = opt_transitive_max_eff;
	transitive_rel_eff  = opt_transitive_rel_eff;
	var_inc				= opt_var_inc;
	var_decay			= opt_var_decay;
	vivify_en			= opt_vivify_en;
	vivify_priorbins	= opt_vivify_priorbins;
	vivify_min_eff		= opt_vivify_min_eff;
	vivify_max_eff		= opt_vivify_max_eff;
	vivify_rel_eff		= opt_vivify_rel_eff;
	walk_priorbins		= opt_walk_priorbins;
	walk_min_eff		= opt_walk_min_eff;
	walk_max_eff		= opt_walk_max_eff;
	walk_rel_eff		= opt_walk_rel_eff;
	report_en			= opt_report_en && !quiet_en;
	reduce_en			= opt_reduce_en;
	reduce_perc			= opt_reduce_perc;
	reduce_inc			= opt_reduce_inc;
	rephase_en			= opt_rephase_en;
	rephase_inc			= opt_rephase_inc;
	restart_inc			= opt_restart_inc;
	stable				= opt_stable;
	stable_rate			= opt_stable_rate;
	preprocess_en		= opt_preprocess_en;
	simplify_en		    = opt_simplify_en;
	simplify_sleep_en	= opt_simplify_sleep_en;
	simplify_inc		= opt_simplify_inc;
	simplify_min		= opt_simplify_min;
	simplify_priorbins	= opt_simplify_priorbins;
	forward_en			= opt_forward_en;
	forward_inc			= opt_forward_inc;
	forward_priorbins	= opt_forward_priorbins;
	forward_max_occs	= opt_forward_max_occs;
	forward_rel_eff		= opt_forward_rel_eff;
	forward_min_eff		= opt_forward_min_eff;
	forward_max_eff		= opt_forward_max_eff;
	forward_max_csize	= opt_forward_max_csize;
	seed				= opt_seed;
	lbd_tier1			= opt_lbd_tier1;
	lbd_tier2			= opt_lbd_tier2;
	lbd_fast			= opt_lbd_fast;
	lbd_slow			= opt_lbd_slow;
	lbd_rate			= opt_lbd_rate;
	luby_inc			= opt_luby_inc;
	luby_max			= opt_luby_max;
	learntsub_max		= opt_learntsub_max;
	gc_perc				= opt_garbage_perc;
	// SAT competition mode
	if (competition_en) {
		assert(proof_path);
		quiet_en = true, report_en = false;
		proof_en = true, proof_nonbinary_en = false;
		model_en = true, modelprint_en = true, modelverify_en = false;
	}
	// initialize simplifier options
	if (preprocess_en || simplify_en) {
		all_en				= opt_all_en;
		ve_en				= opt_ve_en || opt_ve_plus_en;
		ve_plus_en			= opt_ve_plus_en;
		ve_lbound_en		= opt_ve_lbound_en;
		ve_fun_en			= opt_ve_fun_en;
		ve_clause_max		= opt_ve_clause_max;
		xor_max_arity		= opt_xor_max_arity;
		bce_en				= opt_bce_en;
		bce_max_occurs		= opt_bce_max_occurs;
		ere_en				= opt_ere_en;
		ere_extend			= opt_ere_extend;
		ere_max_occurs		= opt_ere_max_occurs;
		ere_clause_max		= opt_ere_clause_max;
		sub_en				= opt_sub_en;
		sub_max_occurs		= opt_sub_max_occurs;
		sub_clause_max		= opt_sub_clause_max;
		lcve_min_vars		= opt_lcve_min_vars;
		lcve_max_occurs		= opt_lcve_max_occurs;
		lcve_clause_max		= opt_lcve_clause_max;
		phase_lits_min		= opt_lits_phase_min;
		mu_pos				= opt_mu_pos;
		mu_neg				= opt_mu_neg;
		phases				= opt_phases;
		solve_en			= opt_solve_en;
		collect_freq		= opt_collect_freq;
		profile_simplifier		= opt_profile_simp_en;
		aggr_cnf_sort		= opt_aggr_cnf_sort;
		if (all_en) 
			ve_en = 1, ve_plus_en = 1, bce_en = 1, ere_en = 1;
		if (!phases && (ve_en || sub_en || bce_en)) 
			phases = 1; // at least 1 phase needed
		if (phases && !(ve_en || sub_en || bce_en))
			phases = 0;
		if (phases > 1 && !ve_en) 
			phases = 1;
	}
}