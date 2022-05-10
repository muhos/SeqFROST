/***********************************************************************[els.cpp]
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

// 'substitute' corresponding clauses having literals represented by 'smallest'
// but start with learnts first which gives priority to substituted learnts in 
// the watch table when new clauses are added

inline uint32 Solver::minReachable(WL& ws, DFS* dfs, const DFS& node) 
{
	uint32 new_min = node.min;
	const State_t* states = sp->state;
	forall_watches(ws, i) {
		const WATCH w = *i;
		if (w.binary()) {
			const uint32 child = w.imp;
			CHECKLIT(child);
			if (states[ABS(child)].state) continue;
			const DFS& child_dfs = dfs[child];
			if (new_min > child_dfs.min) 
				new_min = child_dfs.min;
		}
	}
	return new_min;
}

bool Solver::decompose()
{
	if (UNSAT) return false;
	assert(!LEVEL);
	assert(sp->propagated == trail.size());
	assert(analyzed.empty());
	assert(minimized.empty());
	stats.decompose.calls++;
	const uint32 dfs_size = inf.nDualVars;
	const State_t* states = sp->state;
	DFS* dfs = sfcalloc<DFS>(dfs_size);
	uint32* smallests = sfcalloc<uint32>(dfs_size), dfs_idx = 0;
	uVec1D& scc = analyzed; 
	uVec1D& litstack = minimized;
	uint32 substituted = 0, sccs = 0, before = ACTIVEVARS;
	forall_literals(root) {
		if (UNSAT) break;
		const uint32 root_v = ABS(root);
		if (states[root_v].state) continue;
		if (dfs[root].min == UNDEF_VAR) continue;
		assert(scc.empty());
		assert(litstack.empty());
		litstack.push(root);
		while (NOT_UNSAT && litstack.size()) {
			uint32 parent = litstack.back();
			CHECKLIT(parent);
			DFS& parent_dfs = dfs[parent];
			if (parent_dfs.min == UNDEF_VAR) {
				CHECKLIT(smallests[parent]);
				litstack.pop();
			}
			else { // traverse all binaries 
				assert(!smallests[parent]);
				WL& ws = wt[parent]; // 'ws' holds the negations of 'parent'
				if (parent_dfs.idx) { // all children of parent visited and min reachable found
					litstack.pop(); 
					uint32 new_min = minReachable(ws, dfs, parent_dfs); // find min. reachable from the children of 'parent'
					LOG2(4, " dfs search of parent(%d) with index %d reached minimum %d", l2i(parent), parent_dfs.idx, new_min);
					if (parent_dfs.idx == new_min) { // start of SCC block
						// find the smallest variable to represent this SCC 
						uint32 other, size = 0, smallest = parent;
						assert(scc.size());
						CHECKLIT(smallest);
						const uint32 farent = FLIP(parent);
						uint32 j = scc.size();
						do {
							assert(j > 0);
							other = scc[--j];
							CHECKLIT(other);
							if (NEQUAL(other, farent)) {
								if (ABS(other) < ABS(smallest)) 
									smallest = other;
								size++;
							}
							else {
								LOG2(2, " Conflict as both %d and its negation in the same SCC", l2i(parent));
								enqueueUnit(parent);
								learnEmpty();
							}
						} while (NOT_UNSAT && NEQUAL(other, parent));

						if (NOT_UNSAT) {
							LOG2(4, " New SCC of size %d and smallest variable %d", size, l2i(smallest));
							do {
								assert(scc.size());
								other = scc.back();
								scc.pop();
								CHECKLIT(other);
								dfs[other].min = UNDEF_VAR;
								if (iassumed(ABS(other)))
									smallests[other] = other;
								else {
									smallests[other] = smallest;
									if (NEQUAL(other, smallest)) {
										substituted++;
										LOG2(4, "literal %d in SCC substituted by the smallest %d", l2i(other), l2i(smallest));
									}
								}
							} while (NEQUAL(other, parent));
							sccs += size > 1;
						}
					}
					else 
						parent_dfs.min = new_min;
				}
				else { // traverse children of 'parent'
					dfs_idx++;
					assert(dfs_idx < UNDEF_VAR);
					parent_dfs.idx = parent_dfs.min = dfs_idx;
					scc.push(parent);
					LOG2(4, " traversing all implications of parent(%d) at index %u", l2i(parent), dfs_idx);
					forall_watches(ws, i) {
						const WATCH w = *i;
						if (w.binary()) {
							const uint32 child = w.imp;
							CHECKLIT(child);
							if (states[ABS(child)].state) continue;
							const DFS& child_dfs = dfs[child];
							if (child_dfs.idx) continue;
							litstack.push(child);
						}
					}
				}
			}
		}
	}
	LOG2(2, " Decomposition %lld: %d SCCs found, %d variables substituted (%.2f%%)", 
		stats.decompose.calls, sccs, substituted, percent(substituted, before));
	stats.decompose.scc += sccs;
	stats.decompose.variables += substituted;
	std::free(dfs), dfs = NULL;
	scc.clear();
	litstack.clear();
	bool orgsucc = false, learntsucc = false;
	if (substituted) {
		assert(reduced.empty());
		if (NOT_UNSAT) learntsucc = substitute(learnts, smallests);
		if (NOT_UNSAT) orgsucc = substitute(orgs, smallests);
		if (NOT_UNSAT && reduced.size()) {
			forall_cnf(reduced, i) {
				const C_REF r = *i;
				assert(!cm.deleted(r));
				removeClause(cm[r], r);
			}
			reduced.clear(true);
		}
	}
	if (NOT_UNSAT) {
		PREFETCH_CM(cs, deleted);
		recycleWT(cs, deleted); // must be recycled before BCP
		if (sp->propagated < trail.size() && BCP()) {
			LOG2(2, " Propagation after substitution proved a contradiction");
			learnEmpty();
		}
		if (NOT_UNSAT) {
			assert(UNSOLVED);
			forall_variables(v) {
				if (states[v].state) continue;
				const uint32 p = V2L(v);
				const uint32 other = smallests[p];
				CHECKLIT(other);
				if (NEQUAL(other, p)) {
					assert(other < p);
					const uint32 othervar = ABS(other);
					assert(!MELTED(states[othervar].state));
					assert(!SUBSTITUTED(states[othervar].state));
					if (!states[othervar].state) {
						LOG2(4, " %d substituted to %d", v, othervar);
						markSubstituted(v);
					}
					model.saveBinary(p, FLIP(other));
				}
			}
		}
	}
	std::free(smallests), smallests = NULL;
	return UNSAT || (substituted && (orgsucc || learntsucc));
}

bool Solver::substitute(BCNF& cnf, uint32* smallests)
{
	assert(UNSOLVED);
	assert(learntC.empty());
	bool binaries = false;
	uint32 units = 0;
	uint32 deleted = 0, replaced = 0;
	LIT_ST* marks = sp->marks;
	const LIT_ST* values = sp->value;
	const uint32 cnfsize = cnf.size();
	for (uint32 i = 0; NOT_UNSAT && i < cnfsize; ++i) {
		const C_REF ref = cnf[i];
		if (cm.deleted(ref)) continue;
		CLAUSE& c = cm[ref];
		uint32* lits = c.data();
		int j, size = c.size();
		for (j = 0; j < size; ++j) {
			const uint32 lit = lits[j];
			if (NEQUAL(smallests[lit], lit)) 
				break;
		}
		if (j == size) continue;
		replaced++;
		assert(learntC.empty());
		bool satisfied = false;
		for (int k = 0; !satisfied && k < size; ++k) {
			const uint32 lit = lits[k];
			CHECKLIT(lit);
			LIT_ST val = values[lit];
			if (UNASSIGNED(val)) {
				const uint32 replacement = smallests[lit];
				CHECKLIT(replacement);
				val = values[replacement];
				if (UNASSIGNED(val)) {
					const uint32 repvar = ABS(replacement);
					val = marks[repvar];
					if (UNASSIGNED(val)) {
						marks[repvar] = SIGN(replacement);
						learntC.push(replacement);
					}
					else if (NEQUAL(val, SIGN(replacement))) satisfied = true;
				}
				else if (val) satisfied = true;
			}
			else if (val) satisfied = true;
		}
		if (satisfied) {
			LOGCLAUSE(4, c, "  satisfied after substitution");
			reduced.push(ref);
			deleted++;
		}
		else if (learntC.empty()) {
			LOG2(2, "  learnt empty clause during decomposition");
			learnEmpty();
		}
		else if (learntC.size() == 1) {
			LOG2(4, "  found unit %d after substitution", l2i(learntC[0]));
			enqueueUnit(learntC[0]);
			removeClause(c, ref);
			units++;
		}
		else if (NEQUAL(lits[0], learntC[0]) || NEQUAL(lits[1], learntC[1])) { // watches changed, 'learntC' will be added and watched
			if (opts.proof_en) proof.addClause(learntC);
			if (learntC.size() == 2) binaries = true;
			deleted++;
			uint32 last = cnf.size();
			removeClause(c, ref);
			sp->learntLBD = c.lbd();
			C_REF newref = addClause(learntC, c.learnt());
			LOGCLAUSE(4, cm[newref], "  learnt after substitution");
			assert(cnf[last] == newref);
			cnf[last] = ref;
			cnf[i] = newref;
		}
		else {
			if (opts.proof_en) {
				proof.addClause(learntC);
				proof.deleteClause(c);
			}
			assert(size > 2);
			int k;
			const int learntsize = learntC.size();
			for (k = 2; k < learntsize; ++k) 
				lits[k] = learntC[k];
			int removed = size - k;
			if (removed) {
				LOG2(4, "  only shrinking clause as watches did not change");
				if (k == 2) binaries = true;
				shrinkClause(c, removed);
				if (c.original()) stats.shrunken += removed;
			}
			else if (keeping(c)) 
				mark_subsume(c);
			LOGCLAUSE(4, c, "  substituted");
		}
		unmark_literals(learntC);
		learntC.clear();
	}
	deleted += units;
	stats.decompose.hyperunary += units;
	stats.decompose.clauses += deleted;
	LOG2(2, " Decomposition %lld: %d of clauses replaced %.2f%%, producing %d deleted clauses %.2f%%",
		stats.decompose.calls, replaced, percent(replaced, cnfsize), deleted, percent(deleted, replaced));
	return (units || binaries);
}

void Solver::decompose(const bool& first) 
{
	if (!canDecompose(first)) return;
	int rounds = opts.decompose_min;
	if (MAXCLAUSES > opts.decompose_limit) rounds = 1;
	bool success = true;
	for (int round = 1; success && round <= rounds; ++round)
		success = decompose();
	printStats(success, 'e', CVIOLET0);
}

inline bool Solver::canDecompose(const bool& first)
{
	if (!opts.decompose_en) return false;
	const uint64 clauses = MAXCLAUSES;
	if (first && clauses > opts.decompose_limit) return false;
	return (3 * clauses) < (stats.searchticks + opts.decompose_min_eff);
}