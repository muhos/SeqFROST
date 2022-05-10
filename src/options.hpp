/***********************************************************************[options.hpp]
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

#ifndef __OPTIONS_
#define __OPTIONS_

#include <cstring>
#include "datatypes.hpp"
#include "input.hpp"

namespace SeqFROST {

	struct OPTION {
		//==========================================//
		//             Solver options               //
		//==========================================//
		LIT_ST	polarity;
		//------------------------------------------//
		char*	proof_path;
		//------------------------------------------//
		int64	learntsub_max;
		//------------------------------------------//
		int64	inprocess_min, inprocess_inc;
		uint64	conflict_out, decision_out;
		//------------------------------------------//
		double	var_inc, var_decay;
		double  mab_constant;
		double  chb_step; 
		double  chb_step_decay;
		double  chb_step_min;
		double	stable_rate;
		double	lbd_rate;
		double	gc_perc;
		double	map_perc;
		double	reduce_perc;
		double	ternary_perc;
		//------------------------------------------//
		int		nap;
		int		seed;
		int		stable;
		int		prograte;
		int		mode_inc;
		int		reduce_inc;
		int		restart_inc;
		int		rephase_inc;
		int		decompose_min;
		int		decompose_min_eff;
		int		inprocess_priorbins;
		int		minimize_depth;
		int		minimize_min;
		int		minimize_lbd;
		int		decheuristic;
		int		luby_inc, luby_max;
		int		lbd_tier2, lbd_tier1, lbd_fast, lbd_slow;
		int		mdm_rounds, mdm_inc, mdm_heap_pumps, mdm_vmtf_pumps;
		int		forward_priorbins, forward_inc, forward_min_eff, forward_max_eff, forward_rel_eff, forward_max_csize;
		int		probe_inc, probe_min, probe_min_eff, probe_max_eff, probe_rel_eff;
		int		ternary_priorbins, ternary_min_eff, ternary_max_eff, ternary_rel_eff;
		int		transitive_min_eff, transitive_max_eff, transitive_rel_eff;
		int		vivify_priorbins, vivify_min_eff, vivify_max_eff, vivify_rel_eff;
		int		walk_priorbins, walk_min_eff, walk_max_eff, walk_rel_eff;
		//------------------------------------------//
		uint32  chrono_min;
		uint32  forward_max_occs;
		uint32  mdm_max_occs;
		uint32  decompose_limit;
		//------------------------------------------//
		bool	report_en;
		bool	chrono_en;
		bool	reduce_en;
		bool	vivify_en;
		bool	forward_en;
		bool	rephase_en;
		bool	bumpreason_en;
		bool	boundsearch_en;
		bool	decompose_en;
		bool	debinary_en;
		bool	time_quiet_en;
		bool	transitive_en;
		bool	targetonly_en;
		bool	ternary_en, ternary_sleep_en;
		bool	autarky_en, autarky_sleep_en;
		bool	proof_en, proof_nonbinary_en;
		bool	parseonly_en, parseincr_en;
		bool	probe_en, probehbr_en, probe_sleep_en;
		bool	model_en, modelprint_en, modelverify_en;
		bool	minimize_en, minimizebin_en, minimizeall_en, minimizesort_en;
		bool	mdm_walk_init_en, mdm_walk_en, mdm_mcv_en, mdmassume_en, mdmvsidsonly_en;
		
		//==========================================//
		//             Simplifier options           //
		//==========================================//
		bool	sub_en;
		bool	bce_en;
		bool	ere_en;
		bool	all_en;
		bool	solve_en;
		bool	profile_simplifier;
		bool	aggr_cnf_sort;
		bool	preprocess_en, inprocess_en, inprocess_sleep_en;
		bool	ve_en, ve_plus_en, ve_lbound_en, ve_fun_en;
		//------------------------------------------//
		int		phases;
		int		ere_extend;
		int		collect_freq;
		int		xor_max_arity;
		int		ve_clause_max;
		int		sub_clause_max;
		int		ere_clause_max;
		int		lcve_clause_max;
		int		sub_max_occurs, bce_max_occurs, ere_max_occurs;
		//------------------------------------------//
		uint32	lcve_min_vars, lcve_max_occurs;
		uint32	phase_lits_min;
		uint32	mu_pos, mu_neg;
		//------------------------------------------//
		OPTION();
		~OPTION();
		void init();
	};

}

#endif

