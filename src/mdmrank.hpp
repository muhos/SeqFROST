/***********************************************************************[mdmrank.hpp]
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

#ifndef __MDM_RANK_
#define __MDM_RANK_

#include "sort.hpp"
#include "histogram.hpp"

namespace SeqFROST {

	#define eligible_initial \
	{ \
		LOGN2(2, " Finding eligible decisions at initial round.."); \
		eligible.resize(inf.maxVar); \
		occurs.resize(inf.maxVar + 1); \
		histCNF(orgs, true); \
		histCNF(learnts); \
		write_scores(vars, scores, occs); \
		if (opts.mdm_mcv_en) \
			wolfsort(vars, inf.maxVar, STABLE_MCV(scores)); \
		else \
			wolfsort(vars, inf.maxVar, STABLE_LCV(scores)); \
		LOGDONE(2, 5); \
		if (verbose >= 3) { \
			LOG0(" Eligible decisions:"); \
			for (uint32 i = 0; i < eligible.size(); ++i) { \
				uint32 v = eligible[i]; \
				LOG1("  e[%d]->(v: %d, p: %d, n: %d, s: %d)", i, v, occs[v].ps, occs[v].ns, scores[v]); \
			} \
		} \
	}

	#define eligible_heap(HEAPTYPE) \
	{ \
		LOGN2(2, "  finding " #HEAPTYPE " eligible decisions.."); \
		stats.mdm.HEAPTYPE++; \
		eligible.clear(); \
		occurs.resize(inf.maxVar + 1); \
		histCNF(orgs, true); \
		histCNF(learnts); \
		const State_t* states = sp->state; \
		const LIT_ST* values = sp->value; \
		OCCUR* occs = occurs.data(); \
		uint32* scores = sp->tmpstack; \
		uint32* end = HEAPTYPE ## heap.end(); \
		for (uint32* begin = HEAPTYPE ## heap.data(); begin != end; ++begin) {\
			const uint32 v = *begin;\
			if (!states[v].state && UNASSIGNED(values[V2L(v)])) {\
				eligible.push(v);\
				scores[v] = hist_score(v, occs);\
			}\
		}\
		assert(eligible.size() >= 1);\
		wolfsort(eligible.data(), eligible.size(), STABLE_ACTIVITY(HEAPTYPE.scores, scores));\
		LOGDONE(2, 5); \
		if (verbose >= 3) { \
			LOG0("  eligible " #HEAPTYPE " decisions:"); \
			for (uint32 i = 0; i < eligible.size(); ++i) \
				LOG1("  e[%d]->(s: %d, a: %g)", eligible[i], scores[eligible[i]], HEAPTYPE.scores[eligible[i]]); \
		} \
	}

	#define eligible_queue(QUEUETYPE) \
	{ \
		LOGN2(2, "  finding " #QUEUETYPE " eligible decisions.."); \
		stats.mdm.QUEUETYPE++; \
		eligible.clear(); \
		const State_t* states = sp->state; \
		const LIT_ST* values = sp->value; \
		uint32 freevar = QUEUETYPE.free(); \
		assert(freevar); \
		while (freevar) { \
			if (!states[freevar].state && UNASSIGNED(values[V2L(freevar)])) \
				eligible.push(freevar); \
			freevar = QUEUETYPE.previous(freevar); \
		} \
		assert(eligible.size() >= 1); \
		wolfsort(eligible.data(), eligible.size(), STABLE_BUMP(bumps)); \
		LOGDONE(2, 5); \
		if (verbose >= 3) { \
			LOG0("  eligible " #QUEUETYPE " decisions:"); \
			for (uint32 i = 0; i < eligible.size(); ++i) \
				LOG1("  e[%d]->(b: %lld)", eligible[i], bumps[eligible[i]]); \
		} \
	}

}

#endif