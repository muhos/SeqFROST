/***********************************************************************[xor.hpp]
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

#ifndef __XOR_
#define __XOR_

#include "simplify.hpp" 
using namespace SeqFROST;

#if defined(__linux__) || defined(__CYGWIN__) 
#define COUNTFLIPS(X) while (__builtin_parity(++X)); 
#elif defined(_WIN32)
#define COUNTFLIPS(X) while (__popcnt(++X) & 1); 
#endif

inline void freezeArities(SCNF& scnf, OL& me, OL& other)
{
	forall_occurs(me, i) {
		SCLAUSE& c = scnf[*i];
		if (c.size() > 2 && c.molten())
			c.freeze();
	}
	forall_occurs(other, i) {
		SCLAUSE& c = scnf[*i];
		if (c.size() > 2 && c.molten())
			c.freeze();
	}
}

inline void copyClause(SCLAUSE& c, Lits_t& out_c)
{
	out_c.clear();
	forall_clause(c, k) {
		out_c.push(*k);
	}
}

inline bool checkArity(SCLAUSE& c, uint32* literals, const int& size)
{
	assert(literals);
	const uint32* end = literals + size;
	forall_clause(c, i) {
		const uint32 lit = *i;
		uint32* j;
		for (j = literals; j != end; ++j) {
			if (lit == *j) {
				break;
			}
		}
		if (j == end) return false;
	}
	return true;
}

inline bool makeArity(SCNF& scnf, OT& ot, uint32& parity, uint32* literals, const int& size)
{
	const uint32 oldparity = parity;
	COUNTFLIPS(parity);
	for (int k = 0; k < size; ++k) {
		const uint32 bit = (1UL << k);
		if (NEQUAL(parity & bit, oldparity & bit))
			literals[k] = FLIP(literals[k]);
	}
	// search for an arity clause
	assert(size > 2);
	uint32 best = *literals;
	CHECKLIT(best);
	int minsize = ot[best].size();
	for (int k = 1; k < size; ++k) {
		const uint32 lit = literals[k];
		CHECKLIT(lit);
		int lsize = ot[lit].size();
		if (lsize < minsize) {
			minsize = lsize;
			best = literals[k];
		}
	}
	forall_occurs(ot[best], i) {
		SCLAUSE& c = scnf[*i];
		if (c.original() && c.size() == size && checkArity(c, literals, size)) {
			assert(c.original());
			c.melt();
			return true;
		}
	}
	return false;
}

inline bool find_XOR_gate(const uint32& dx, OL& dx_list, 
						  const uint32& fx, OL& fx_list,
						  SCNF& scnf,
						  OT& ot,
						  Lits_t& out_c,
						  const int& orgCls, 
								int& nAddedCls,
								int& nAddedLits)
{	
	assert(dx_list.size());
	assert(fx_list.size());

	if (scnf[dx_list.back()].size() == 2 ||
		scnf[fx_list.back()].size() == 2)
		return false;

	const int maxarity = solver->opts.xor_max_arity;

	int arity = scnf[*dx_list].size() - 1;
	if (arity > maxarity) return false;

	assert(checkMolten(scnf, dx_list, fx_list));

	const uint32 v = ABS(dx);

	forall_occurs(dx_list, i) {
		SCLAUSE& ci = scnf[*i];
		if (ci.original()) {

			const int size = ci.size();
			arity = size - 1; 

			if (size < 3 || arity > maxarity) continue;

			// share to out_c
			copyClause(ci, out_c);

			// find arity clauses
			uint32 parity = 0;
			int itargets = (1 << arity);

			while (--itargets && makeArity(scnf, ot, parity, out_c, size));

			assert(parity < (1UL << size)); // overflow check
			assert(itargets >= 0);

			if (itargets)
				freezeArities(scnf, dx_list, fx_list);
			else {

				ci.melt();

				// check resolvability
				nAddedCls = 0, nAddedLits = 0;
				if (countSubstituted(v, orgCls, scnf, dx_list, fx_list, nAddedCls, nAddedLits)) {
					freezeArities(scnf, dx_list, fx_list);
					break;
				}

				// can be substituted
				#ifdef LOGGING
				if (verbose >= 4) {
					LOGN1(" Gate %d = XOR(", l2i(dx));
					for (int k = 0; k < out_c.size(); ++k) {
						PRINT(" %d", ABS(out_c[k]));
						if (k < out_c.size() - 1) PUTCH(',');
					}
					PRINT(" ) found ==> added = %d, deleted = %d\n", nAddedCls, dx_list.size() + fx_list.size());
					printGate(scnf, dx_list, fx_list);
				}
				#endif		

				return true;
			}
		} // original block
	} // for block

	return false;
}

#endif