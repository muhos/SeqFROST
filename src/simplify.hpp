/***********************************************************************[simplify.hpp]
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

#ifndef __SIMPLIFY_
#define __SIMPLIFY_

#include "solve.hpp" 

using namespace SeqFROST;

#define RESOLUTION 1
#define SUBSTITUTION 2
#define CORESUBSTITUTION 4

struct CNF_CMP_KEY {
	const SCNF& scnf;
	CNF_CMP_KEY(const SCNF& scnf) : scnf(scnf) {}
	inline bool operator () (const S_REF& a, const S_REF& b) {
		const SCLAUSE& x = scnf[a];
		const SCLAUSE& y = scnf[b];
		const int xsize = x.size(), ysize = y.size();
		if (xsize < ysize) return true;
		if (xsize > ysize) return false;
		uint32 xkey = x[0], ykey = y[0];
		if (xkey < ykey) return true;
		if (xkey > ykey) return false;
		xkey = x.back(), ykey = y.back();
		if (xkey < ykey) return true;
		if (xkey > ykey) return false;
		return x.sig() < y.sig();
	}
};

struct STABLE_CNF_KEY {
	const SCNF& scnf;
	STABLE_CNF_KEY(const SCNF& scnf) : scnf(scnf) {}
	inline bool operator () (const S_REF* a, const S_REF* b) {
		const SCLAUSE& x = scnf[*a];
		const SCLAUSE& y = scnf[*b];
		const int xsize = x.size(), ysize = y.size();
		if (xsize < ysize) return true;
		if (xsize > ysize) return false;
		uint32 xkey = x[0], ykey = y[0];
		if (xkey < ykey) return true;
		if (xkey > ykey) return false;
		xkey = x.back(), ykey = y.back();
		if (xkey < ykey) return true;
		if (xkey > ykey) return false;
		xkey = x.sig(), ykey = y.sig();
		if (xkey < ykey) return true;
		if (xkey > ykey) return false;
		return *a < *b;
	}
};

inline void printGate(const SCNF& scnf, const OL& poss, const OL& negs)
{
	for (int i = 0; i < poss.size(); ++i) {
		const SCLAUSE& c = scnf[poss[i]];
		if (c.molten()) {
			LOGN0(" ");
			c.print();
		}
	}
	for (int i = 0; i < negs.size(); ++i) {
		const SCLAUSE& c = scnf[negs[i]];
		if (c.molten()) {
			LOGN0(" ");
			c.print();
		}
	}
}

inline bool checkDeleted(const SCNF& scnf, const OL& poss, const OL& negs)
{
	for (int i = 0; i < poss.size(); ++i) {
		const SCLAUSE& c = scnf[poss[i]];
		if (c.deleted()) 
			return false;
	}
	for (int i = 0; i < negs.size(); ++i) {
		const SCLAUSE& c = scnf[negs[i]];
		if (c.deleted()) 
			return false;
	}
	return true;
}

inline bool checkMolten(const SCNF& scnf, const OL& poss, const OL& negs)
{
	for (int i = 0; i < poss.size(); ++i) {
		const SCLAUSE& c = scnf[poss[i]];
		if (!c.deleted() && c.molten())
			return false;
	}
	for (int i = 0; i < negs.size(); ++i) {
		const SCLAUSE& c = scnf[negs[i]];
		if (!c.deleted() && c.molten())
			return false;
	}
	return true;
}

inline bool sub(const uint32& A, const uint32& B) { return !(A & ~B); }

inline bool selfsub(const uint32& A, const uint32& B)
{
	uint32 B_tmp = B | ((B & 0xAAAAAAAAUL) >> 1) | ((B & 0x55555555UL) << 1);
	return !(A & ~B_tmp);
}

inline bool isEqual(SCLAUSE& c1, Lits_t& c2)
{
	assert(!c1.deleted());
	int it = 0;
	const int size = c2.size();
	assert(c1.size() == size);
	uint32* lits1 = c1.data();
	uint32* lits2 = c2.data();
	while (it < size) {
		if (NEQUAL(lits1[it], lits2[it]))
			return false;
		else 
			it++;
	}
	return true;
}

inline void cswap(uint32& x, uint32& y)
{
	const uint32 ta = MIN(x, y);
	const uint32 tb = MAX(x, y);
	x = ta, y = tb;
}

inline void sort3(uint32& x, uint32& y, uint32& z)
{
	cswap(y, z);
	cswap(x, z);
	cswap(x, y);
	assert(x <= y && y <= z && x <= z);
}

inline bool isTautology(const uint32& x, const SCLAUSE& c1, const SCLAUSE& c2)
{
	CHECKVAR(x);
	assert(c1.original());
	assert(c2.original());
	const int n1 = c1.size(), n2 = c2.size();
	int it1 = 0, it2 = 0;
	while (it1 < n1 && it2 < n2) {
		const uint32 lit1 = c1[it1], lit2 = c2[it2];
		CHECKLIT(lit1);
		CHECKLIT(lit2);
		const uint32 v1 = ABS(lit1), v2 = ABS(lit2);
		if (v1 == x) { it1++; }
		else if (v2 == x) { it2++; }
		else if ((lit1 ^ lit2) == NEG_SIGN) return true; 
		else if (v1 < v2) it1++;
		else if (v2 < v1) it2++;
		else { it1++; it2++; }
	}
	return false;
}

inline bool merge(const uint32& x, SCLAUSE& c1, SCLAUSE& c2, Lits_t& out_c)
{
	CHECKVAR(x);
	assert(c1.original());
	assert(c2.original());

	out_c.clear();

	const int n1 = c1.size(), n2 = c2.size();
	uint32* d1 = c1.data();
	uint32* d2 = c2.data();
	uint32* e1 = d1 + n1;
	uint32* e2 = d2 + n2;
	uint32 lit1, lit2, v1, v2;
	while (d1 != e1 && d2 != e2) {
		lit1 = *d1;
		lit2 = *d2;
		CHECKLIT(lit1);
		CHECKLIT(lit2);
		v1 = ABS(lit1);
		v2 = ABS(lit2);
		if (v1 == x) { d1++; }
		else if (v2 == x) { d2++; }
		else if ((lit1 ^ lit2) == NEG_SIGN) return false;
		else if (v1 < v2) { d1++; out_c.push(lit1); }
		else if (v2 < v1) { d2++; out_c.push(lit2); }
		else { // repeated literal
			assert(lit1 == lit2);
			d1++, d2++;
			out_c.push(lit1);
		}
	}
	while (d1 != e1) {
		lit1 = *d1++;
		CHECKLIT(lit1);
		if (NEQUAL(ABS(lit1), x)) 
			out_c.push(lit1);
	}
	while (d2 != e2) {
		lit2 = *d2++;
		CHECKLIT(lit2);
		if (NEQUAL(ABS(lit2), x))
			out_c.push(lit2);
	}
	return true;
}

inline int merge(const uint32& x, SCLAUSE& c1, SCLAUSE& c2)
{
	CHECKVAR(x);
	assert(c1.original());
	assert(c2.original());
	const int n1 = c1.size(), n2 = c2.size();
	uint32* d1 = c1.data();
	uint32* d2 = c2.data();
	uint32* e1 = d1 + n1;
	uint32* e2 = d2 + n2;
	uint32 lit1, lit2, v1, v2;
	int len = n1 + n2 - 2;
	while (d1 != e1 && d2 != e2) {
		lit1 = *d1;
		lit2 = *d2;
		CHECKLIT(lit1);
		CHECKLIT(lit2);
		v1 = ABS(lit1);
		v2 = ABS(lit2);
		if (v1 == x) d1++;
		else if (v2 == x) d2++;
		else if ((lit1 ^ lit2) == NEG_SIGN) return 0;
		else if (v1 < v2) d1++;
		else if (v2 < v1) d2++;
		else { // repeated literal
			assert(lit1 == lit2);
			d1++, d2++;
			assert(len > 0);
			len--;
		}
	}
	return len;
}

inline void freezeBinaries(SCNF& scnf, OL& list)
{
	forall_occurs(list, i) {
		SCLAUSE& c = scnf[*i];
		if (c.original() && c.size() == 2)
			c.freeze();
	}
}

inline void freezeAll(SCNF& scnf, OL& list)
{
	forall_occurs(list, i) {
		SCLAUSE& c = scnf[*i];
		if (c.original()) 
			c.freeze();
	}
}

inline void countOrgs(SCNF& scnf, OL& list, int& orgs)
{
	assert(!orgs);
	forall_occurs(list, i) {
		if (scnf[*i].original()) 
			orgs++;
	}
}

inline void countLitsBefore(SCNF& scnf, OL& list, int& nLitsBefore)
{
	forall_occurs(list, i) {
		SCLAUSE& c = scnf[*i];
		if (c.original()) 
			nLitsBefore += c.size();
	}
}

inline bool countSubstituted(const uint32& x, const int& clsbefore, SCNF& scnf, OL& me, OL& other, 
	int& nAddedCls, int& nAddedLits)
{
	assert(!nAddedCls);
	assert(!nAddedLits);
	const int rlimit = solver->opts.ve_clause_max;
	forall_occurs(me, i) {
		SCLAUSE& ci = scnf[*i];
		if (ci.original()) {
			const bool a = ci.molten();
			forall_occurs(other, j) {
				SCLAUSE& cj = scnf[*j];
				if (cj.original()) {
					const bool b = cj.molten();
					int rsize;
					if (a != b && (rsize = merge(x, ci, cj)) > 1) {
						if (++nAddedCls > clsbefore || (rlimit && rsize > rlimit)) return true;
						nAddedLits += rsize;
					}
				}
			}
		}
	}
	if (solver->opts.ve_lbound_en) {
		int nLitsBefore = 0;
		countLitsBefore(scnf, me, nLitsBefore);
		countLitsBefore(scnf, other, nLitsBefore);
		if (nAddedLits > nLitsBefore) return true;
	}
	return false;
}

inline bool countResolvents(const uint32& x, SCNF& scnf, OL& me, OL& other, 
	int& nAddedCls, int& nAddedLits)
{
	assert(!nAddedCls);
	assert(!nAddedLits);
	const int rlimit = solver->opts.ve_clause_max;
	forall_occurs(me, i)  {
		SCLAUSE& ci = scnf[*i];
		if (ci.original()) {
			forall_occurs(other, j){
				SCLAUSE& cj = scnf[*j];
				if (cj.original()) {
					int rsize;
					if ((rsize = merge(x, ci, cj)) > 1) {
						if (rlimit && rsize > rlimit) return false;
						nAddedCls++;
						nAddedLits += rsize;
					}
				}
			}
		}
	}
	return true;
}

inline bool countResolvents(const uint32& x, const int& clsbefore, SCNF& scnf, OL& me, OL& other, 
	int& nAddedCls, int& nAddedLits)
{
	assert(!nAddedCls);
	assert(!nAddedLits);
	const int rlimit = solver->opts.ve_clause_max;
	forall_occurs(me, i)  {
		SCLAUSE& ci = scnf[*i];
		if (ci.original()) {
			forall_occurs(other, j){
				SCLAUSE& cj = scnf[*j];
				if (cj.original()) {
					int rsize;
					if ((rsize = merge(x, ci, cj)) > 1) {
						if (++nAddedCls > clsbefore || (rlimit && rsize > rlimit)) return false;
						nAddedLits += rsize;
					}
				}
			}
		}
	}
	if (solver->opts.ve_lbound_en) {
		int nLitsBefore = 0;
		countLitsBefore(scnf, me, nLitsBefore);
		countLitsBefore(scnf, other, nLitsBefore);
		if (nAddedLits > nLitsBefore) return false;
	}
	return true;
}

inline void toblivion(SCNF& scnf, OL& list)
{
	forall_occurs(list, i) {
		scnf[*i].markDeleted();
	}
	list.clear(true);
}

inline void toblivion(const uint32& p, OL& poss,
					  const uint32& n, OL& negs, 
					  const int& pOrgs, 
					  const int& nOrgs, 
					  SCNF& scnf, 
					  MODEL& model)
{
	LOG2(4, " saving clauses of eliminated(%d) as witness", ABS(p));
	if (pOrgs > nOrgs) {
		forall_occurs(negs, i) {
			SCLAUSE& c = scnf[*i];
			if (c.original())
				model.saveClause(c, c.size(), n);
			c.markDeleted();
		}
		model.saveWitness(p);
		negs.clear(true);
		toblivion(scnf, poss);
	}
	else {
		forall_occurs(poss, i) {
			SCLAUSE& c = scnf[*i];
			if (c.original())
				model.saveClause(c, c.size(), p);
			c.markDeleted();
		}
		model.saveWitness(n);
		poss.clear(true);
		toblivion(scnf, negs);
	}
}

#endif