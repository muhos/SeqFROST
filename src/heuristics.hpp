/***********************************************************************[heuristics.hpp]
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

#ifndef __HEURISTICS_
#define __HEURISTICS_

#include "definitions.hpp"
#include "heap.hpp"
#include "key.hpp"

namespace SeqFROST {

	typedef HEAP<ACTIV_CMP> dheap_t;

	struct VSIDS {

		score_t scores;
		double inc, booster;
		double min, max;

		VSIDS() : 
			inc(0)
			, booster(0)
			, min(1e-150)
			, max(1e150)
		{}

		~VSIDS() { scores.clear(true); }

		inline void init(const double& _inc, const double& _decay) {
			assert(inf.maxVar);
			inc = _inc;
			booster = 1.0 / _decay;
			assert(inc <= 1 && booster >= 1);
		}

		inline void boost() {
			assert(booster >= 1);
			inc *= booster;
		}

		inline void scale() {
			assert(min > 0 && min < 1);
			double* s = scores.data();
			forall_variables(v) {
				s[v] *= min;
			}
			inc *= min;
			assert(inc <= (1.0 / min));
		}

		inline void	bump(dheap_t& heap, const uint32& v) {
			CHECKVAR(v);
			assert(scores[v] <= max);
			if ((scores[v] += inc) > max)
				scale();
			assert(scores[v] <= max);
			heap.update(v);
		}

		inline void	bump(dheap_t& heap, const uint32& v, const double& bonus) {
			CHECKVAR(v);
			assert(scores[v] <= max);
			if ((scores[v] += (inc * bonus)) > max)
				scale();
			assert(scores[v] <= max);
			heap.update(v);
		}

	};

	struct CHB {

		Vec<uint64> conflicts;
		score_t scores;
		double step;
		double step_decay;
		double step_min;

		CHB() : 
			step(0)
			, step_decay(0)
			, step_min(0)
		{}

		~CHB() { 
			scores.clear(true);
			conflicts.clear(true);
		}

		inline void init(const double& _s, const double& _d, const double& _m) { 
			step = _s;
			step_min = _m;
			step_decay = _d;
		}

		inline void decay(const bool& allowed) {
			if (allowed && step > step_min) 
				step -= step_decay;
		}

		inline void	bump(dheap_t& heap, const uint32& v, const uint64& nConflicts, const double& multiplier) {
			assert(nConflicts >= conflicts[v]);
			const uint64 age = nConflicts - conflicts[v] + 1;
			const double reward = multiplier / age;
			const double oldscore = scores[v];
			const double newscore = step * reward + (1 - step) * oldscore;
			scores[v] = newscore;
			heap.update(v, oldscore, newscore);
		}

		inline void	bump(const uint32& v, const double& val) {
			CHECKVAR(v);
			assert(val);
			assert(scores[v] == 0.0);
			scores[v] = val;
		}

	};

	constexpr uint32 ARMS = 2;

	struct MAB {

		double constant;
		double decisions;
		double ucb_0, ucb_1;
		double reward[ARMS];
		uint32 select[ARMS];
		uint32 chosen;
		
		MAB() : 
			constant(0)
			, decisions(0)
			, ucb_0(0)
			, ucb_1(0)
			, reward{ 0, 0 }
			, select{ 0, 0 }
			, chosen(0)
		{
			assert(ARMS == 2);
		}

		void init(const double& _constant, const int& heuristic) { 
			constant = _constant;

			reward[0] = 0;
			select[0] = 0;
			reward[1] = 0;
			select[1] = 0;

			select[heuristic]++; 
		}

		void restart(int& decheuristic)
		{
			uint32 selections = 0;

			if (chosen)
				reward[decheuristic] += log2(decisions) / chosen;

			chosen = 0;
			decisions = 0;

			selections += select[0];
			selections += select[1];

			if (selections < ARMS)
				decheuristic = !decheuristic;
			else {

				++selections;

				const double m = constant * log(selections);

				uint32 n = select[0];
				ucb_0 = reward[0] / n + sqrt(m / n);
				n = select[1];
				ucb_1 = reward[1] / n + sqrt(m / n);

				if (ucb_1 > ucb_0)
					decheuristic = 1;
				else 
					decheuristic = 0;
			}

			select[decheuristic]++;
		}	
	};

}

#endif