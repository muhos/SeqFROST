/***********************************************************************[probe.cpp]
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

void Solver::probe()
{
	rootify();
	assert(conflict == UNDEF_REF);
	assert(UNSOLVED);
	assert(!probed);

	stats.probe.calls++;

	printStats(1, '-', CVIOLET0);

#ifdef LOGGING
	const uint32 before = ACTIVEVARS;
#endif

	probed = true;

	decompose(true);
	ternary();  
	debinary();
	transitive();
	failing();
	vivify();
	decompose(false);

	probed = false;

#ifdef LOGGING
	const uint32 after = ACTIVEVARS;
	const uint32 removed = before - after;
	assert(removed >= 0);
	LOG2(2, " Probe call %lld removed %d variables %.2f%%", stats.probe.calls, removed, percent(removed, before));
#endif

	INCREASE_LIMIT(probe, stats.probe.calls, nlogn, true);
	last.probe.reduces = stats.reduces + 1;
}