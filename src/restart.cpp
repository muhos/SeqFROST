/***********************************************************************[restart.cpp]
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

using namespace SeqFROST;

void Solver::restart()
{
	assert(sp->propagated == trail.size());
	assert(conflict == UNDEF_REF);
	assert(UNSOLVED);
	stats.restart.all++;

	const int before = decheuristic;
	if (stable) {
		stats.mab.restarts++;
		forall_variables(v) {
			sp->state[v].mab = 0;
		}
		mab.restart(decheuristic); // modifies decheuristic
	}
	const int after = decheuristic;

	const uint32 btlevel = before == after ? reuse() : 0;
	if (stable) decheuristic = before;
	backtrack(btlevel);
	if (stable) decheuristic = after;

	if (stable) {
		stats.restart.stable++;
		if (before != after) 
			updateHeap();
	}
	else 
		updateUnstableLimit();
}

void Solver::updateUnstableLimit()
{
	assert(!stable);
	uint64 increase = opts.restart_inc - 1;
	increase += logn(stats.restart.all);
	limit.restart.conflicts = stats.conflicts + increase;
}

uint32 Solver::reuse() 
{
	const level_t* dlevels = dlevel.data();
	uint32 target = 0;
	if (stable) {
		const uint32 cand = nextHeap(DECISIONHEAP);
		assert(cand);
		ACTIV_CMP hcmp(HEAPSCORES);
		while (target < LEVEL) {
			const uint32 decision = dlevels[target + 1].decision;
			CHECKLIT(decision);
			if (hcmp(cand, ABS(decision)))
				target++;
			else break;
		}
	}
	else {
		const uint32 cand = nextQueue();
		assert(cand);
		const uint64 candBump = bumps[cand];
		while (target < LEVEL) {
			const uint32 decision = dlevels[target + 1].decision;
			CHECKLIT(decision);
			if (candBump < bumps[ABS(decision)])
				target++;
			else break;
		}
	}
	if (target) stats.reuses++;
	return target;
}

#if defined(_WIN32)
#pragma warning(push)
#pragma warning(disable : 4146)
#endif

void LUBYREST::update()
{
	if (!period || restart) return;
	assert(countdown > 0);
	if (--countdown) return;
	if ((u & -u) == v) u++, v = 1;
	else v <<= 1;
	assert(v);
	assert((UINT64_MAX / v) >= period);
	countdown = v * period;
	restart = true;
	// reset if limit is reached
	if (limited && countdown > limit) {
		u = v = 1;
		countdown = period;
	}
}

#if defined(_WIN32)
#pragma warning(pop)
#endif