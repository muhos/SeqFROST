/***********************************************************************[bounded.cpp]
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

#include "and.hpp"
#include "xor.hpp"
#include "equivalence.hpp"
#include "ifthenelse.hpp"
#include "function.hpp"

using namespace SeqFROST;

void Solver::VE()
{
	if (!opts.ve_en) return;
	LOG2(2, " Eliminating variables..");
	if (INTERRUPTED) killSolver();
	if (opts.profile_simplifier) timer.pstart();
	Lits_t out_c;
	out_c.reserve(opts.ve_clause_max);

#ifdef STATISTICS
	BVESTATS& bvestats = stats.inprocess.bve;
#endif

	const bool firstcall = stats.inprocess.calls == 1;
	forall_vector(uint32, elected, i) {
		uint32 v = *i;
		CHECKVAR(v);
		assert(!sp->state[v].state);
		const uint32 p = V2L(v), n = NEG(p);
		OL& poss = ot[p], &negs = ot[n];
		assert(isSorted(poss.data(), poss.size(), CNF_CMP_KEY(scnf)));
		assert(isSorted(negs.data(), negs.size(), CNF_CMP_KEY(scnf)));
		int pOrgs = 0, nOrgs = 0;
		int nAddedCls = 0, nAddedLits = 0;
		if (firstcall) {
			pOrgs = poss.size();
			nOrgs = negs.size();
		}
		else {
			assert(stats.inprocess.calls > 1);
			countOrgs(scnf, poss, pOrgs);
			countOrgs(scnf, negs, nOrgs);
		}
		// pure-literal
		if (!pOrgs || !nOrgs) {
			toblivion(p, poss, n, negs, pOrgs, nOrgs, scnf, model);
#ifdef STATISTICS
			bvestats.pures++;
#endif
			v = 0;
		}
		// Equiv/NOT-gate Reasoning
		else if (uint32 def = find_BN_gate(p, n, scnf, poss, negs)) {
#ifdef STATISTICS
			bvestats.inverters++;
#endif
			save_BN_gate(p, pOrgs, nOrgs, scnf, poss, negs, model);
			if (substitute_single(p, def, scnf, ot)) {
				LOG2(2, "  BVE proved a contradiction");
				learnEmpty();
				killSolver();
			}
			v = 0;
		}
		// simple resolution case 
		else if ((pOrgs == 1 || nOrgs == 1) && countResolvents(v, scnf, poss, negs, nAddedCls, nAddedLits)) {
			if (nAddedCls) resolve(v, out_c, nAddedCls, nAddedLits);
			toblivion(p, poss, n, negs, pOrgs, nOrgs, scnf, model);
#ifdef STATISTICS
			bvestats.resolutions++;
#endif
			v = 0;
		}
		else {
			assert(pOrgs && nOrgs);
			nAddedCls = 0;
			nAddedLits = 0;
			out_c.clear();
			const int orgCls = pOrgs + nOrgs;
			Byte type = 0;
			if (orgCls > 2) {
				// AND/OR-gate Reasoning
				if (find_AO_gate(n, negs, p, poss, scnf, out_c, orgCls, nAddedCls, nAddedLits)) {
					type = SUBSTITUTION;
					#ifdef STATISTICS
					bvestats.andors++;
					#endif
				}
				else if (!nAddedCls && find_AO_gate(p, poss, n, negs, scnf, out_c, orgCls, nAddedCls, nAddedLits)) {
					type = SUBSTITUTION;
					#ifdef STATISTICS
					bvestats.andors++;
					#endif
				}
			}
			if (!type && orgCls > 3) {
				// ITE-gate Reasoning
				if (find_ITE_gate(p, poss, n, negs, scnf, ot, orgCls, nAddedCls, nAddedLits)) {
					type = SUBSTITUTION;
					#ifdef STATISTICS
					bvestats.ites++;
					#endif
				}
				else if (!nAddedCls && find_ITE_gate(n, negs, p, poss, scnf, ot, orgCls, nAddedCls, nAddedLits)) {
					type = SUBSTITUTION;
					#ifdef STATISTICS
					bvestats.ites++;
					#endif
				}
				// XOR-gate Reasoning
				else if (find_XOR_gate(p, poss, n, negs, scnf, ot, out_c, orgCls, nAddedCls, nAddedLits)) {
					type = SUBSTITUTION;
					#ifdef STATISTICS
					bvestats.xors++;
					#endif
				}
				else if (!nAddedCls && find_XOR_gate(n, negs, p, poss, scnf, ot, out_c, orgCls, nAddedCls, nAddedLits)) {
					type = SUBSTITUTION;
					#ifdef STATISTICS
					bvestats.xors++;
					#endif
				}
			}
			// fun-gate reasoning
			if (!type && orgCls > 2 && find_fun_gate(p, orgCls, scnf, ot, nAddedCls, nAddedLits)) {
				if (UNSAT) {
					LOG2(2, "  BVE proved a contradiction during fun search");
					if (opts.proof_en) proof.addEmpty();
					killSolver();
				}
				type = CORESUBSTITUTION;
				#ifdef STATISTICS
				bvestats.funs++;
				#endif
			}
			// n-by-m resolution
			else if (!type && !nAddedCls && countResolvents(v, orgCls, scnf, poss, negs, nAddedCls, nAddedLits)) {
				type = RESOLUTION;
				#ifdef STATISTICS
				bvestats.resolutions++;
				#endif
			}
			//=======================
			// resolve or substitute
			//=======================
			if (type & SUBSTITUTION) {
				if (nAddedCls) substitute(v, out_c, nAddedCls, nAddedLits);
				toblivion(p, poss, n, negs, pOrgs, nOrgs, scnf, model);
				v = 0;
			}
			else if (type & CORESUBSTITUTION) {
				if (nAddedCls) resolveCore(v, out_c, nAddedCls, nAddedLits);
				toblivion(p, poss, n, negs, pOrgs, nOrgs, scnf, model);
				v = 0;
			}
			else if (type & RESOLUTION) {
				if (nAddedCls) resolve(v, out_c, nAddedCls, nAddedLits);
				toblivion(p, poss, n, negs, pOrgs, nOrgs, scnf, model);
				v = 0;
			}
		}
		if (!v) {
			markEliminated(*i);
			*i = 0;
		}
	}
	if (opts.profile_simplifier) timer.pstop(), timer.ve += timer.pcpuTime();
	LOGREDALL(this, 2, "VE Reductions");
}

inline void Solver::substitute(const uint32& x, Lits_t& out_c, const int& nAddedCls, const int& nAddedLits)
{
	CHECKVAR(x);
	LOG2(4, " Substituting(%d):", x);
	LOGOCCURS(this, 4, x);
	int checksum = 0;
	S_REF ref;
	S_REF* refs = scnf.alloc(ref, nAddedCls, nAddedLits);
	uint32 dx = V2L(x), fx = NEG(dx);

	if (ot[dx].size() > ot[fx].size()) std::swap(dx, fx);

	OL& me = ot[dx], & other = ot[fx];
	forall_occurs(me, i) {
		SCLAUSE& ci = scnf[*i];
		if (ci.original()) {
			const bool a = ci.molten();
			forall_occurs(other, j) {
				SCLAUSE& cj = scnf[*j];
				if (cj.original()) {
					const bool b = cj.molten();
					if (NEQUAL(a, b) && merge(x, ci, cj, out_c)) {
						addResolvent(out_c, ref);		
						refs[checksum++] = ref, ref += SCBUCKETS(out_c.size());
					}						
				}
			}
		}
	}
	assert(checksum == nAddedCls);
}

inline void Solver::resolveCore(const uint32& x, Lits_t& out_c, const int& nAddedCls, const int& nAddedLits)
{
	CHECKVAR(x);
	LOG2(4, " Resolving(%d) using core:", x);
	LOGOCCURS(this, 4, x);
	int checksum = 0;
	S_REF ref;
	S_REF* refs = scnf.alloc(ref, nAddedCls, nAddedLits);
	uint32 dx = V2L(x), fx = NEG(dx);

	if (ot[dx].size() > ot[fx].size()) std::swap(dx, fx);

	OL& me = ot[dx], & other = ot[fx];
	forall_occurs(me, i) {
		SCLAUSE& ci = scnf[*i];
		if (ci.original()) {
			const bool a = ci.molten();
			forall_occurs(other, j) {
				SCLAUSE& cj = scnf[*j];
				if (cj.original()) {
					const bool b = cj.molten();
					if ((!a || !b) && merge(x, ci, cj, out_c)) {
						addResolvent(out_c, ref);		
						refs[checksum++] = ref, ref += SCBUCKETS(out_c.size());
					}
				}
			}
		}
	}
	assert(checksum == nAddedCls);
}

inline void Solver::resolve(const uint32& x, Lits_t& out_c, const int& nAddedCls, const int& nAddedLits)
{
	CHECKVAR(x);
	LOG2(4, " Resolving(%d):", x);
	LOGOCCURS(this, 4, x);
	int checksum = 0;
	S_REF ref;
	S_REF* refs = scnf.alloc(ref, nAddedCls, nAddedLits);
	uint32 dx = V2L(x), fx = NEG(dx);

	if (ot[dx].size() > ot[fx].size()) std::swap(dx, fx);

	OL& me = ot[dx], & other = ot[fx];
	forall_occurs(me, i) {
		SCLAUSE& ci = scnf[*i];
		if (ci.original()) {
			forall_occurs(other, j) {
				SCLAUSE& cj = scnf[*j];
				if (cj.original() && merge(x, ci, cj, out_c)) {
					addResolvent(out_c, ref);		
					refs[checksum++] = ref, ref += SCBUCKETS(out_c.size());
				}
			}
		}
	}
	assert(checksum == nAddedCls);
}

inline void Solver::addResolvent(const Lits_t& resolvent, const S_REF& ref)
{
	const int size = resolvent.size();
	assert(size);
	if (size == 1) {
		const uint32 unit = resolvent[0];
		CHECKLIT(unit);
		const LIT_ST val = sp->value[unit];
		if (UNASSIGNED(val)) {
			enqueueUnit(unit);
		}
		else if (!val) {
			LOG2(2, "  BVE proved a contradiction");
			learnEmpty();
			killSolver();
		}
	}
	else {
		SCLAUSE* added = new (scnf.clause(ref)) SCLAUSE(resolvent);
		assert(added->size() == size);
		assert(added->hasZero() < 0);
		assert(added->original());
		assert(added->isSorted());
		if (opts.proof_en)
			proof.addResolvent(*added);
		added->calcSig();
		added->markAdded();
		LOGCLAUSE(4, (*added), " Resolvent");
	}
}