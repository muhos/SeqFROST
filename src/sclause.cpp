/***********************************************************************[sclause.cpp]
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

void Solver::addClause(SCLAUSE& src)
{
	int size = src.size();
	assert(size > 1);
	assert(!src.deleted());
	assert(!src.molten());	
	// NOTE: 'src' should be used before any mapping is done
	if (stats.inprocess.calls > 1 && src.added()) {
		mark_ssubsume(src);
	}
	C_REF r = UNDEF_REF;
	CLAUSE& dest = *cm.alloc(r, size);
	if (mapped) 
		vmap.mapClause(dest, src);
	else 
		dest.copyLits(src);
	assert(size == dest.size());
	assert(dest.keep());
	assert(dest[0] > 1 && dest[1] > 1);
	assert(dest[0] <= UNDEF_VAR && dest[1] <= UNDEF_VAR);
	assert(!cm.deleted(r));
	assert(src.status() == ORIGINAL || src.status() == LEARNT);
	if (src.learnt()) {
		assert(src.lbd());
		assert(src.usage() < USAGET1);
		assert(!src.added());
		dest.markLearnt();
		int lbd = src.lbd();
		if (size > 2 && lbd > opts.lbd_tier1) 
			dest.set_keep(0);
		dest.set_lbd(lbd);
		dest.set_usage(src.usage());
		learnts.push(r);
		stats.literals.learnt += size;
	}
	else {
		assert(dest.original());
		orgs.push(r);
		stats.literals.original += size;
	}
}
