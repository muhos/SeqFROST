/***********************************************************************[redundancy.hpp]
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

#ifndef __ERE_
#define __ERE_

#include "simplify.hpp"

using namespace SeqFROST;

inline int merge_ere(const uint32& x, 
					 SCLAUSE& c1, 
					 SCLAUSE& c2, 
					 const OT& ot,
					 const int maxlen, 
					 Lits_t& out_c, 
					 uint32& sig, 
					 uint32& best)
{
	CHECKVAR(x);
	assert(!c1.deleted());
	assert(!c2.deleted());
	
	out_c.clear();
	sig = 0;
	best = 0;

	int lsize = INT_MAX, minsize = INT_MAX;
	uint32* d1 = c1.data();
	uint32* d2 = c2.data();
	uint32* e1 = c1.end();
	uint32* e2 = c2.end();
	uint32 lit1, lit2, v1, v2;
	while (d1 != e1 && d2 != e2) {
		if (out_c.size() > maxlen) 
			return 0;
		lit1 = *d1;
		lit2 = *d2;
		CHECKLIT(lit1);
		CHECKLIT(lit2);
		v1 = ABS(lit1);
		v2 = ABS(lit2);
		if (v1 == x) d1++;
		else if (v2 == x) d2++;
		else if ((lit1 ^ lit2) == NEG_SIGN) return 0;
		else if (v1 < v2) {
			d1++;
			lsize = ot[lit1].size();
			if (lsize < minsize) 
				minsize = lsize, best = lit1;
			sig |= MAPHASH(lit1);
			out_c.push(lit1);
		}
		else if (v2 < v1) {
			d2++;
			lsize = ot[lit2].size();
			if (lsize < minsize) 
				minsize = lsize, best = lit2;
			sig |= MAPHASH(lit2);
			out_c.push(lit2);
		}
		else { 
			assert(lit1 == lit2);
			d1++, d2++;
			lsize = ot[lit1].size();
			if (lsize < minsize) 
				minsize = lsize, best = lit1;
			sig |= MAPHASH(lit1);
			out_c.push(lit1);
		}
	}

	while (d1 != e1) {
		lit1 = *d1++;
		CHECKLIT(lit1);
		if (NEQUAL(ABS(lit1), x)) {
			lsize = ot[lit1].size();
			if (lsize < minsize) 
				minsize = lsize, best = lit1;
			sig |= MAPHASH(lit1);
			out_c.push(lit1);
		}
	}

	if (out_c.size() > maxlen)
		return 0;

	while (d2 != e2) {
		lit2 = *d2++;
		CHECKLIT(lit2);
		if (NEQUAL(ABS(lit2), x)) {
			lsize = ot[lit2].size();
			if (lsize < minsize) 
				minsize = lsize, best = lit2;
			sig |= MAPHASH(lit2);
			out_c.push(lit2);
		}
	}

	const int size = out_c.size();

	if (size > maxlen)
		return 0;

	return size;
}

#endif


