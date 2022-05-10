/***********************************************************************[ifthenelse.hpp]
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

#ifndef __ITE_
#define __ITE_

#include "simplify.hpp"

using namespace SeqFROST;

inline S_REF fast_equality_check(SCNF& scnf, OT& ot, uint32 x, uint32 y, uint32 z)
{
	if (ot[y].size() > ot[z].size()) std::swap(y, z);
	if (ot[x].size() > ot[y].size()) std::swap(x, y);

	OL& list = ot[x];

	sort3(x, y, z);

	forall_occurs(list, i) {
		SCLAUSE& c = scnf[*i];
		if (c.molten()) continue;
		assert(c.isSorted());
		const uint32* lits = c.data();
		if (c.original() && c.size() == 3 &&
			lits[0] == x && lits[1] == y && lits[2] == z) 
			return *i;
	}

	return UNDEF_REF;
}

inline bool find_ITE_gate(const uint32& dx, OL& dx_list, 
						  const uint32& fx, OL& fx_list,
						  SCNF& scnf,
						  OT& ot, 
						  const int& orgCls, 
								int& nAddedCls, 
								int& nAddedLits)
{
	assert(dx_list.size());

	if (scnf[dx_list.back()].size() == 2)
		return false;

	assert(checkMolten(scnf, dx_list, fx_list));

	const uint32 v = ABS(dx);

	S_REF* end = dx_list.end();
	for (S_REF* i = dx_list; i != end; ++i) {
		SCLAUSE& ci = scnf[*i];
		if (ci.original() && ci.size() == 3) {
			uint32 xi = ci[0], yi = ci[1], zi = ci[2];
			CHECKLIT(xi);
			CHECKLIT(yi);
			CHECKLIT(zi);
			if (yi == dx) std::swap(xi, yi);
			if (zi == dx) std::swap(xi, zi);
			assert(xi == dx);
			for (S_REF* j = i + 1; j != end; ++j) {
				SCLAUSE& cj = scnf[*j];
				if (cj.original() && cj.size() == 3) {
					assert(cj.original());
					uint32 xj = cj[0], yj = cj[1], zj = cj[2];
					CHECKLIT(xj);
					CHECKLIT(yj);
					CHECKLIT(zj);
					if (yj == dx) std::swap(xj, yj);
					if (zj == dx) std::swap(xj, zj);
					assert(xj == dx);
					if (ABS(yi) == ABS(zj)) std::swap(yj, zj);
					if (ABS(zi) == ABS(zj)) continue;
					if (yi != FLIP(yj)) continue;

					S_REF d1 = fast_equality_check(scnf, ot, fx, yi, FLIP(zi));
					if (d1 == UNDEF_REF) continue;
					S_REF d2 = fast_equality_check(scnf, ot, fx, yj, FLIP(zj));
					if (d2 == UNDEF_REF) continue;
					assert(scnf[d1].original());
					assert(scnf[d2].original());

					// mark gate clauses
					ci.melt(), cj.melt();
					scnf[d1].melt(), scnf[d2].melt();

					// check resolvability
					nAddedCls = 0, nAddedLits = 0;
					if (countSubstituted(v, orgCls, scnf, dx_list, fx_list, nAddedCls, nAddedLits)) {
						ci.freeze(), cj.freeze();
						scnf[d1].freeze(), scnf[d2].freeze();
						return false;
					}

					// can be substituted
					#ifdef LOGGING
					if (verbose >= 4) {
						LOG1(" Gate %d = ITE(%d, %d, %d) found ==> added = %d, deleted = %d",
							l2i(dx), -int(ABS(yi)), -int(ABS(zi)), -int(ABS(zj)), 
							nAddedCls, dx_list.size() + fx_list.size());
						printGate(scnf, dx_list, fx_list);
					}
					#endif	

					return true;
				}
			}
		}
	}
	return false;
}

#endif