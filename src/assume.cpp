/***********************************************************************[assume.cpp]
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

bool Solver::ifailed(const uint32& v)
{
	const uint32 mlit = imap(v);
	if (!mlit) return false;
	CHECKLIT(mlit);
	const int size = iconflict.size();
	assert(size);
	for (int i = 0; i < size; ++i) {
		if (ABS(mlit) == v)
			return true;
	}
	return false;
}

bool Solver::ieliminated(const uint32& v)
{
	const uint32 mlit = imap(v);
	if (!mlit) return true;
	CHECKLIT(mlit);
	return MELTED(sp->state[ABS(mlit)].state) || SUBSTITUTED(sp->state[ABS(mlit)].state);
}

void Solver::ifreeze(const uint32& v) 
{
	const uint32 mlit = imap(v);
	CHECKLIT(mlit);
	const uint32 mvar = ABS(mlit);
	ifrozen[mvar] = 1;
	LOG2(3, "  freezing original variable %d (mapped to %d)..", v, mvar);
}

void Solver::iunfreeze(const uint32& v)
{
	const uint32 mlit = imap(v);
	CHECKLIT(mlit);
	const uint32 mvar = ABS(mlit);
	ifrozen[mvar] = 0;
	LOG2(3, "  melting original variable %d (mapped to %d)..", v, mvar);
}

#if defined(__linux__) || defined(__CYGWIN__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

void Solver::iassume(Lits_t& assumptions)
{
	assert(inf.maxVar);
	assert(ORIGINALS);
	if (assumptions.empty()) return;
	LOGN2(2, " Adding %d assumptions..", assumptions.size());
	this->assumptions.reserve(assumptions.size());
	forall_clause(assumptions, k) {
		const uint32 a = *k, v = ABS(a);
		CHECKLIT(a);
		assert(!ieliminated(v));
		assert(!ifrozen[v]);
		ifrozen[ABS(a)] = 1;
		this->assumptions.push(a);
	}
	LOGDONE(2, 5);
}

#if defined(__linux__) || defined(__CYGWIN__)
#pragma GCC diagnostic pop
#endif

void Solver::iunassume()
{
	assert(inf.maxVar);
	assert(ORIGINALS);
	LOGN2(2, " Resetting %d assumptions and solver state..", assumptions.size());
	if (assumptions.size()) {
		forall_clause(assumptions, k) {
			const uint32 a = *k;
			CHECKLIT(a);
			ifrozen[ABS(a)] = 0;
		}
		assumptions.clear(true);
	}
	RESETSTATE;
	LOGDONE(2, 5);
	backtrack();
}

void Solver::idecide()
{
	assert(inf.unassigned);
	assert(sp->propagated == trail.size());
	assert(conflict == UNDEF_REF);
	assert(UNSOLVED);
	uint32 dec = 0;
	while (LEVEL < assumptions.size()) {
		const uint32 a = assumptions[LEVEL];
		CHECKLIT(a);
		assert(ifrozen[ABS(a)]);
		const LIT_ST val = sp->value[a];
		if (UNASSIGNED(val)) {
			dec = a;
			break;
		}
		else if (val) {
			INCLEVEL(a); 
		}
		else {
			assert(!val);
			ianalyze(FLIP(a));
			SET_UNSAT;
			return;
		}
	}
	if (!dec) {
		const uint32 cand = stable ? nextHeap(DECISIONHEAP) : nextQueue();
		dec = makeAssign(cand, TARGETING);
	}
	enqueueDecision(dec);
	stats.decisions.single++;
}

void Solver::ianalyze(const uint32& failed)
{
	assert(UNSAT);
	assert(vorg);
	LOG2(3, " Analyzing conflict on failed assumption (%d):", l2i(failed));
	LOGTRAIL(this, 3);
	iconflict.clear();
	iconflict.push(failed);
	if (!LEVEL) return;
	sp->seen[ABS(failed)] = ANALYZED_M;
	assert(trail.size());
	uint32* tail = trail.end() - 1, *pivot = trail + dlevel[1].depth;
	while (tail >= pivot) {
		const uint32 parent = *tail--;
		CHECKLIT(parent);
		const uint32 parentv = ABS(parent);
		if (sp->seen[parentv]) {
			const C_REF r = sp->source[parentv];
			if (REASON(r)) {
				CLAUSE& c = cm[r];
				LOGCLAUSE(4, c, "  analyzing %d reason", l2i(parent));
				forall_clause(c, k) {
					const uint32 other = *k;
					if (other == parent) continue;
					const uint32 v = ABS(other);
					CHECKVAR(v);
					if (sp->level[v]) 
						sp->seen[v] = ANALYZED_M;
				}
			}
			else {
				assert(sp->level[parentv]);
				iconflict.push(FLIP(parent));
			}
			sp->seen[parentv] = 0;
		}
	}
	sp->seen[ABS(failed)] = 0;
}