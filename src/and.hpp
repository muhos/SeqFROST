/***********************************************************************[and.hpp]
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

#ifndef __AND_
#define __AND_

#include "simplify.hpp"
#include "sort.hpp"

using namespace SeqFROST;

inline void find_fanin(const uint32& gate_out, SCNF& scnf, OL& list, Lits_t& out_c, uint32& sig)
{
	CHECKLIT(gate_out);
	out_c.clear();
	sig = 0;
	uint32 imp = 0;
	forall_occurs(list, i) {
		SCLAUSE& c = scnf[*i];
		assert(!c.molten());
		if (c.original() && c.size() == 2) {
			imp = FLIP(c[0] ^ c[1] ^ gate_out);
			CHECKLIT(imp);
			out_c.push(imp);
			sig |= MAPHASH(imp);
			c.melt(); // mark as gate clause
		}
	}
}

inline bool find_AO_gate(const uint32& dx, OL& dx_list, 
						 const uint32& fx, OL& fx_list, 
						 SCNF& scnf,
						 Lits_t& out_c,
						 const int& orgCls, 
							   int& nAddedCls,
							   int& nAddedLits)
{
	assert(dx_list.size());
	assert(fx_list.size());

	if (scnf[*dx_list].size() > 2 || scnf[fx_list.back()].size() < 3)
		return false;

	assert(checkMolten(scnf, dx_list, fx_list));

	uint32 sig;
	find_fanin(dx, scnf, dx_list, out_c, sig);

	if (out_c.size() > 1) {
		
		out_c.push(fx);

		sig |= MAPHASH(fx);

		SORT(out_c);

		const uint32 x = ABS(dx);

		forall_occurs(fx_list, i) {

			SCLAUSE& c = scnf[*i];

			if (c.original() && c.size() == out_c.size() && sub(c.sig(), sig) && isEqual(c, out_c)) {

				c.melt(); // mark as fanout clause

				// check resolvability
				nAddedCls = 0, nAddedLits = 0;
				if (countSubstituted(x, orgCls, scnf, dx_list, fx_list, nAddedCls, nAddedLits)) {
					c.freeze();
					break;
				}

				// can be substituted
				#ifdef LOGGING
				if (verbose >= 4) {
					LOGN1(" Gate %d = %s(", x, SIGN(dx) ? "AND" : "OR");
					for (int k = 0; k < out_c.size(); ++k) {
						if (ABS(out_c[k]) == x) continue;
						PRINT(" %d", ABS(out_c[k]));
						if (k < out_c.size() - 1) PUTCH(',');
					}
					PRINT(" ) found ==> added = %d, deleted = %d\n", nAddedCls, dx_list.size() + fx_list.size());
					printGate(scnf, dx_list, fx_list);
				}
				#endif

				return true;
			}
		}
	}

	freezeBinaries(scnf, dx_list);

	return false;
}

#endif