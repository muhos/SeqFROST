/***********************************************************************[minimize.cpp]
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

inline int Solver::calcLBD()
{
	assert(!learntC.empty());
	if (learntC.size() == 1) return 0;
	else if (learntC.size() == 2) return (l2dl(learntC[0]) != l2dl(learntC[1]));
	else {
		int lbd = 0;
		const uint32 marker = ++stats.marker;
		forall_clause(learntC, k) {
			const uint32 litLevel = l2dl(*k);
			if (NEQUAL(sp->board[litLevel], marker)) { 
				sp->board[litLevel] = marker;
				lbd++;
			}
		}
		return lbd - 1;
	}
}

void Solver::minimizebin()
{
	const uint32 uip = FLIP(learntC[0]);
	const uint32 unmarker = stats.marker++, marker = stats.marker;
	uint32* board = sp->board;
	uint32 *j, *end = learntC.end();
	for (j = learntC + 1; j != end; ++j)
		board[ABS(*j)] = marker;
	WL& ws = wt[uip];
	int nLitsRem = 0;
	const int size = learntC.size() - 1;
	forall_watches(ws, i) {
		const WATCH w = *i;
		if (w.binary()) {
			const uint32 other = w.imp, v = ABS(other);
			if (board[v] == marker && isTrue(other)) {
				board[v] = unmarker;
				sp->seen[v] = 0; // see no evil!
				nLitsRem++;
			}
		}
		if (nLitsRem == size) break; // bail out early
	}
	if (nLitsRem) {
		assert(end == learntC.end());
		j = end;
		uint32* i, *newend = j - nLitsRem;
		for (i = learntC + 1; i != newend; ++i) {
			const uint32 lit = *i;
			if (NEQUAL(board[ABS(lit)], marker))
				*i-- = *--j;
		}
		learntC.shrink(nLitsRem);
		sp->learntLBD = calcLBD();
		assert(sp->learntLBD <= opts.lbd_tier2);
		LOG2(4, " Learnt clause strengthened by %d binaries with new LBD %d", nLitsRem, sp->learntLBD);
	}
}