/***********************************************************************[minimize.hpp]
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

#ifndef __MINIMIZEALL_
#define __MINIMIZEALL_

#include "definitions.hpp"
#include "space.hpp"

namespace SeqFROST {

	struct MINIMIZE_CMP {
		const SP* sp;
		MINIMIZE_CMP(const SP* sp) : sp(sp) {}
		inline bool operator()(const uint32& a, const uint32& b) {
			const uint32 va = ABS(a), vb = ABS(b);
			const uint32 la = sp->level[va], lb = sp->level[vb];
			if (la > lb) return true;
			if (la < lb) return false;
			return sp->trailpos[va] > sp->trailpos[vb];
		}
	};

	struct LEARNTLIT {
		uint32 lit, idx;
		LEARNTLIT() {}
		LEARNTLIT(const uint32& lit, const uint32& idx) :
			lit(lit), idx(idx) {}
	};

	struct LEARNTLIT_CMP {
		const SP* sp;
		LEARNTLIT_CMP(const SP* sp) : sp(sp) {}
		inline bool operator()(const LEARNTLIT& a, const LEARNTLIT& b) {
			const uint32 va = ABS(a.lit), vb = ABS(b.lit);
			const uint32 la = sp->level[va], lb = sp->level[vb];
			if (la > lb) return true;
			if (la < lb) return false;
			return sp->trailpos[va] > sp->trailpos[vb];
		}
	};

	inline LEARNTLIT* nextLitBlock(const SP* sp, LEARNTLIT* learntstart, LEARNTLIT* bend, uint32& level, uint32& maxtrail)
	{
		level = UNDEF_LEVEL;
		maxtrail = 0;
		LEARNTLIT* btail = bend;
		while (btail > learntstart) {
			const uint32 lit = (--btail)->lit;
			CHECKLIT(lit);
			const uint32 v = ABS(lit);
			const uint32 litlevel = sp->level[v];
			if (level == UNDEF_LEVEL)
				level = litlevel;
			else if (litlevel > level)
				return btail + 1;
			else
				assert(litlevel == level);
			const uint32 trailPos = sp->trailpos[v];
			if (trailPos > maxtrail)
				maxtrail = trailPos;
		}
		return btail;
	}

}

#endif