/***********************************************************************[failing.cpp]
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

using namespace SeqFROST;

struct PROBE_QUEUE_CMP {
	const State_t* states;
	const bump_t& bumped;
	PROBE_QUEUE_CMP(const State_t* _states, const bump_t& _bumped) :
		states(_states), bumped(_bumped) {}
	inline uint64 operator () (const uint32& a, const uint32& b) const {
		const uint32 av = ABS(a), bv = ABS(b);
		const bool pa = states[av].probe, pb = states[bv].probe;
		if (!pa && pb) return true;
		if (pa && !pb) return false;
		return bumped[av] < bumped[bv];
	}
};

struct PROBE_HEAP_CMP {
	const State_t* states;
	const score_t& act;
	PROBE_HEAP_CMP(const State_t* _states, const score_t& _act) :
		states(_states), act(_act) {}
	inline bool operator () (const uint32& a, const uint32& b) const {
		const uint32 av = ABS(a), bv = ABS(b);
		const bool pa = states[av].probe, pb = states[bv].probe;
		if (!pa && pb) return true;
		if (pa && !pb) return false;
		const double xact = act[av], yact = act[bv];
		if (xact < yact) return true;
		if (xact > yact) return false;
		return av < bv;
	}
};

void Solver::scheduleProbes() 
{
	assert(probes.empty());
	vhist.resize(inf.nDualVars, 0);
	histBins(orgs);
	histBins(learnts);
	const uint32* hist = vhist.data();
	const State_t* states = sp->state;
	uint32 count[2] = { 0 , 0 };
	forall_variables(v) {
		if (states[v].state) continue;
		const uint32 p = V2L(v), n = NEG(p);
		const uint32 poss = hist[p], negs = hist[n];
		if ((poss && negs) || (!poss && !negs)) continue;
		const uint32 probe = negs ? p : n;
	#ifdef LOGGING
		LOG2(4, "  scheduling probe %d with binary occurs %d", l2i(probe), vhist[FLIP(probe)]);
	#endif
		probes.push(probe);
		assert(states[v].probe <= 1);
		count[states[v].probe]++;
	}
	assert(probes.size() == count[0] + count[1]);
	LOG2(2, "  scheduled %d (%d prioritized) probes %.2f%%", probes.size(), count[1], percent(probes.size(), ACTIVEVARS));
}

inline uint32 Solver::nextProbe() 
{
	while (probes.size()) {
		const uint32 probe = probes.back();
		CHECKLIT(probe);
		probes.pop();
		if (!sp->state[ABS(probe)].state) 
			return probe;
	}
	return 0;
}

void Solver::failing()
{
	if (UNSAT) return;
	assert(!LEVEL);
	assert(sp->propagated == trail.size());

	SLEEPING(sleep.probe, opts.probe_sleep_en);
	SET_BOUNDS(probe_limit, probe, probeticks, searchticks, nlogn(ACTIVEVARS));

	const LIT_ST* values = sp->value;
	State_t* states = sp->state;

	ignore = UNDEF_REF;

	uint64 oldhypers = stats.binary.resolvents;
	uint32 probe = 0, currprobed = 0, currfailed = 0;
	for (int round = 1; round <= opts.probe_min; ++round) {

		scheduleProbes();

		if (probes.size()) {
			if (stable) 
				QSORTCMP(probes, PROBE_HEAP_CMP(states, HEAPSCORES));
			else 
				QSORTCMP(probes, PROBE_QUEUE_CMP(states, bumps));
		}
		else {
			LOG2(2, "  no candidates found to probe");
			break;
		}

		uint32* numfailed = vhist.data();
		memset(numfailed, 0, sizeof(uint32) * inf.nDualVars);

		stats.probe.rounds++;

		currprobed = currfailed = 0;
		while ((probe = nextProbe())
			&& stats.probeticks < probe_limit
			&& NOT_UNSAT && !INTERRUPTED)
		{
			assert(!LEVEL);
			assert(unassigned(probe));
			assert(sp->propagated == trail.size());
			states[ABS(probe)].probe = 0;
			if (currfailed && numfailed[probe] == currfailed) continue;
			currprobed++;
			enqueueDecision(probe);
			const uint32 propagated = sp->propagated;
			if (BCPProbe()) {
				currfailed++;
				analyze();
				assert(!LEVEL);
				if (UNASSIGNED(values[probe]))
					enqueueUnit(FLIP(probe));
				if (BCP()) {
					LOG2(2, "  failed probe %d proved a contradiction", l2i(probe));
					learnEmpty();
				}
			}
			else {
				assert(LEVEL == 1);
				assert(sp->propagated == trail.size());
				const uint32 trailsize = trail.size();
				for (uint32 i = propagated; i < trailsize; ++i)
					numfailed[trail[i]] = currfailed;
				backtrack();
			}
		}

		stats.probe.probed += currprobed;
		stats.probe.failed += currfailed;

		if (UNSAT) {
			LOG2(2, "  probing proved a contradiction");
			break;
		}

		LOG2(2, " Probe round %lld: probed %d, finding %d failed literals and %lld hyper binary resolvents",
			stats.probe.rounds, currprobed, currfailed, stats.binary.resolvents - oldhypers);

		const uint32 remained = probes.size();
		if (remained) {
			LOG2(2, "  probing hit limit at round %d with %d remaining probes", round, remained);
			bool prioritized = false;
			for (uint32 i = 0; !prioritized && i < probes.size(); ++i) {
				if (states[ABS(probes[i])].probe)
					prioritized = true;
			}
			if (!prioritized) {
				LOG2(2, "  prioritizing remaining %d probes at round %d", remained, round);
				while (probes.size()) {
					const uint32 probe = probes.back();
					CHECKLIT(probe);
					probes.pop();
					assert(!states[ABS(probe)].probe);
					states[ABS(probe)].probe = 1;
				}
			}
			break;
		}

		if (!currfailed) break;
	}

	vhist.clear(true);
	probes.clear(true);
	assert(stats.binary.resolvents >= oldhypers);
	uint64 currhypers = stats.binary.resolvents - oldhypers;
	const bool success = currfailed || currhypers;
	UPDATE_SLEEPER(probe, success);
	printStats(success, 'f', CVIOLET4);
}