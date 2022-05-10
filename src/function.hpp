/***********************************************************************[function.hpp]
Copyright(c) 2021, Muhammad Osama - Anton Wijs,
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

#ifndef __FUNCTION_
#define __FUNCTION_

#include "simplify.hpp"

using namespace SeqFROST;

constexpr uint64 ALLONES = ~0;

inline void falsefun(Fun f)
{
	for (uint32 i = 0; i < FUNTABLEN; ++i)
		f[i] = 0ULL;
}

inline void truefun(Fun f) 
{
	for (uint32 i = 0; i < FUNTABLEN; ++i)
		f[i] = ALLONES;
}

inline bool isfalsefun(const Fun f) 
{
	for (uint32 i = 0; i < FUNTABLEN; ++i)
		if (f[i]) return false;
	return true;
}

inline bool istruefun(const Fun f) 
{
	for (uint32 i = 0; i < FUNTABLEN; ++i)
		if (f[i] != ALLONES) return false;
	return true;
}

inline void orfun(Fun a, const Fun b) 
{
	for (uint32 i = 0; i < FUNTABLEN; ++i)
		a[i] |= b[i];
}

inline void andfun(Fun a, const Fun b) 
{
	for (uint32 i = 0; i < FUNTABLEN; ++i)
		a[i] &= b[i];
}

inline void copyfun(Fun a, const Fun b) 
{
	for (uint32 i = 0; i < FUNTABLEN; ++i)
		a[i] = b[i];
}

inline void clause2fun(const int& v, const bool& sign, Fun f) 
{
	assert(v >= 0 && v < MAXFUNVAR);
	if (v < 6) {
		uint64 val = magicnumbers[v];
		if (sign) val = ~val;
		for (uint32 i = 0; i < FUNTABLEN; ++i)
			f[i] |= val;
	}
	else {
		uint64 val = sign ? ALLONES : 0ULL;
		int j = 0;
		int sv = 1 << (v - 6);
		for (uint32 i = 0; i < FUNTABLEN; ++i) {
			f[i] |= val;
			if (++j >= sv) {
				val = ~val;
				j = 0;
			}
		}
	}
}

inline bool countMinResolvents(const uint32& x, const int& clsbefore, SCNF& scnf, OL& me, OL& other, 
	int& nAddedCls, int& nAddedLits)
{
	// 'molten' this time marks a non-gate clause 
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
					if ((!a || !b) && (rsize = merge(x, ci, cj)) > 1) {
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

inline bool buildfuntab(const uint32& lit, SCNF& scnf, OT& ot, Fun f)
{
	CHECKLIT(lit);
	LOG2(4, "  building function table for %d environment", l2i(lit));

	Fun cls;

	truefun(f);

	OL& list = ot[lit];
	forall_occurs(list, i) {
		SCLAUSE& c = scnf[*i];
		if (c.learnt()) continue;
		assert(!c.deleted());
		LOGCLAUSE(4, c, "   anding");

		falsefun(cls);

		forall_clause(c, k) {
			const uint32 other = *k;
			if (other == lit) continue;
			assert(other != FLIP(lit));
			const LIT_ST mvar = solver->l2marker(other); // mvar[0 : MAXFUNVAR - 1]
			assert(mvar < MAXFUNVAR);
			if (mvar < 0) return false; // out-of-bounds mapped variable
			clause2fun(mvar, SIGN(other), cls);
		}

		assert(!isfalsefun(cls));
		assert(!istruefun(cls));

		andfun(f, cls);
	}
	return true;
}

inline void buildfuntab(const uint32& lit, const int& tail, SCNF& scnf, const OL& ol, Fun fun, bool& core)
{
	CHECKLIT(lit);
	LOGCLAUSE(4, scnf[ol[tail]], "   ignoring");

	Fun cls;

	for (int j = 0; j < tail; ++j) {
		SCLAUSE& c = scnf[ol[j]];
		if (c.learnt()) continue;
		assert(!c.deleted());
		LOGCLAUSE(4, c, "   anding");

		falsefun(cls);

		forall_clause(c, k) {
			const uint32 other = *k;
			if (other == lit) continue;
			assert(other != FLIP(lit));
			const LIT_ST mvar = solver->l2marker(other);
			assert(mvar >= 0 && mvar < MAXFUNVAR);
			clause2fun(mvar, SIGN(other), cls);
		}

		assert(!isfalsefun(cls));
		assert(!istruefun(cls));

		andfun(fun, cls);
	}

	if (isfalsefun(fun)) {
		scnf[ol[tail]].melt(); // non-gate clause (not resolved with its kind)
		core = true;
	}
}

inline uint64 collapsefun(uint64& allzero, const Fun b, const Fun c) 
{
	assert(!allzero);
	uint64 unsat = 0;
	for (uint32 i = 0; i < FUNTABLEN; ++i) {
		unsat |= (b[i] | c[i]);
		allzero |= (b[i] & c[i]);
	}
	return unsat;
}

inline bool find_fun_gate(const uint32& p, const int& orgCls, SCNF& scnf, OT& ot, 
	int& nAddedCls, int& nAddedLits)
{
	if (!solver->opts.ve_fun_en) return 0;
	const uint32 n = NEG(p);
	Fun pos, neg;
	if (buildfuntab (p, scnf, ot, pos) && buildfuntab (n, scnf, ot, neg)) {
		uint64 allzero = 0;
		if (!collapsefun(allzero, pos, neg)) {
			solver->cnfstate = UNSAT_M;
			return true;
		}
		if (!allzero) {
			Fun& fun = pos;
			LOG2 (4, "  extracting core for %d..", l2i(p));
			OL& poss = ot[p];
			int start = poss.size() - 1;
			bool core = false;
			for (int i = start; i >= 0; --i) {
				copyfun(fun, neg);
				if (scnf[poss[i]].original()) 
					buildfuntab(p, i, scnf, poss, fun, core);
			}
			OL& negs = ot[n];
			start = negs.size() - 1;
			for (int i = start; i >= 0; --i) {
				truefun(fun);
				if (scnf[negs[i]].original()) 
					buildfuntab(n, i, scnf, negs, fun, core);
			}
			LOGOCCURS(solver, 4, ABS(p));
			// check addibility
			nAddedCls = 0, nAddedLits = 0;
			if (countMinResolvents(ABS(p), orgCls, scnf, poss, negs, nAddedCls, nAddedLits)) {
				if (core) {
					freezeAll(scnf, poss);
					freezeAll(scnf, negs);
				}
				return false;
			}
			return true;
		}
	}
	return false;
}

#endif