/***********************************************************************[recycle.cpp]
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

inline void Solver::moveClause(C_REF& r, CMM& newBlock, const cbucket_t* cs)
{
	GET_CLAUSE(c, r, cs);
	if (c.moved()) { 
		r = c.ref(); 
		return;
	}
	newBlock.alloc(r, c);
	c.set_ref(r);
}

inline void	Solver::moveWatches(WL& ws, CMM& newBlock, const cbucket_t* cs)
{
	forall_watches(ws, w) {
		moveClause(w->ref, newBlock, cs);
	}
	ws.shrinkCap();
}

inline void	Solver::recycleWL(const uint32& lit, const cbucket_t* cs, const bool* deleted)
{
	CHECKLIT(lit);
	WL& ws = wt[lit];
	if (ws.empty()) return;
	WL hypers;
	const uint32 fit = FLIP(lit);
	WATCH *j = ws;
	forall_watches(ws, i) {
		WATCH w = *i;
		const C_REF r = w.ref;
		assert(r != UNDEF_REF);
		if (deleted[r]) continue;
		GET_CLAUSE(c, r, cs);
		w.imp = c[0] ^ c[1] ^ fit;
		w.size = c.size();
		if (c.binary()) {
			if (c.hyper()) 
				hypers.push(w);
			else
				*j++ = w;
		}
		else if (c.original())
			*j++ = w;
	}
	ws.resize(uint32(j - ws));
	forall_watches(hypers, i) 
		ws.push(*i);
	hypers.clear(true);
}

void Solver::markReasons() 
{
	PREFETCH_CS(cs);
	const State_t* states = sp->state;
	const C_REF* sources = sp->source;
	forall_vector(uint32, trail, t) {
		const uint32 lit = *t, v = ABS(lit);
		if (states[v].state) continue;
		assert(!unassigned(lit));
		assert(sp->level[v]);
		CHECKLEVEL(sp->level[v]);
		const C_REF r = sources[v];
		if (REASON(r)) {
			GET_CLAUSE(c, r, cs);
			assert(!c.reason());
			c.markReason();
		}
	}
}

void Solver::unmarkReasons() 
{
	PREFETCH_CS(cs);
	const State_t* states = sp->state;
	const C_REF* sources = sp->source;
	forall_vector(uint32, trail, t) {
		const uint32 lit = *t, v = ABS(lit);
		if (states[v].state) continue;
		assert(!unassigned(lit));
		assert(sp->level[v]);
		CHECKLEVEL(sp->level[v]);
		const C_REF r = sources[v];
		if (REASON(r)) {
			GET_CLAUSE(c, r, cs);
			assert(c.reason());
			c.initReason();
		}
	}
}

void Solver::recycleWT(const cbucket_t* cs, const bool* deleted) 
{
	forall_variables(v) {
		const uint32 p = V2L(v), n = NEG(p);
		recycleWL(p, cs, deleted);
		recycleWL(n, cs, deleted);
	}

	forall_cnf(learnts, i) {
		const C_REF r = *i;
		assert(r < cm.size());
		if (deleted[r]) continue;
		GET_CLAUSE(c, r, cs);
		if (c.binary()) continue;
		sortClause(c);
		ATTACH_TWO_WATCHES(r, c);
	}
}

void Solver::recycle(CMM& new_cm)
{
#ifdef STATISTICS
	uint64 bytesBefore =
		(orgs.capacity() + learnts.capacity() + reduced.capacity()) * sizeof(C_REF) +
		(analyzed.capacity() + minimized.capacity()) * sizeof(uint32);
#endif

	reduced.clear(true);
	analyzed.clear(true);
	minimized.clear(true);

	PREFETCH_CM(cs, deleted);

	recycleWT(cs, deleted);

	for (uint32 q = vmtf.last(); q; q = vmtf.previous(q)) {
		const uint32 lit = makeAssign(q), fit = FLIP(lit);
		moveWatches(wt[lit], new_cm, cs);
		moveWatches(wt[fit], new_cm, cs);
	}

	C_REF* sources = sp->source;
	const uint32* levels = sp->level;
	forall_vector(uint32, trail, t) {
		const uint32 lit = *t, v = ABS(lit);
		C_REF& r = sources[v];
		if (REASON(r)) {
			if (levels[v]) {
				assert(r < cm.size());
				if (deleted[r]) 
					r = UNDEF_REF;
				else 
					moveClause(r, new_cm, cs);
			}
			else r = UNDEF_REF;
		}
	}

	filter(orgs, new_cm, cs, deleted);
	filter(learnts, new_cm, cs, deleted);

	orgs.shrinkCap();

#ifdef STATISTICS
	uint64 bytesAfter = (orgs.capacity() + learnts.capacity()) * sizeof(C_REF) +
						(analyzed.capacity() + minimized.capacity()) * sizeof(uint32);
	assert(bytesBefore >= bytesAfter);
	stats.recycle.saved += bytesBefore - bytesAfter;
#endif
}

void Solver::recycle() 
{
	assert(sp->propagated == trail.size());
	assert(conflict == UNDEF_REF);
	assert(UNSOLVED);

	shrink();

	if (cm.garbage() > (cm.size() * opts.gc_perc)) {
		LOGN2(2, " Recycling garbage..");
		stats.recycle.hard++;
		assert(cm.size() >= cm.garbage());
		const size_t bytes = cm.size() - cm.garbage();
		CMM new_cm(bytes);
		recycle(new_cm);
#ifdef STATISTICS
		assert(cm.capacity() >= new_cm.capacity());
		stats.recycle.saved += (cm.capacity() - new_cm.capacity()) * cm.bucket();
#endif
		LOGENDING(2, 5, "(%.3f KB collected)", ratio((double)(cm.garbage() * cm.bucket()), (double)KBYTE)); 
		new_cm.migrateTo(cm);
	}
	else {
		stats.recycle.soft++;
		PREFETCH_CM(cs, deleted);
		recycleWT(cs, deleted);
		filter(learnts, deleted);
	}
}

void Solver::filter(BCNF& cnf, const bool* deleted) 
{
	if (cnf.empty()) return;
	C_REF* j = cnf;
	forall_cnf(cnf, i) {
		const C_REF r = *i;
		if (deleted[r]) continue;
		*j++ = r;
	}
	assert(j >= cnf);
	cnf.resize(uint32(j - cnf));
}

void Solver::filter(BCNF& cnf, CMM& new_cm, const cbucket_t* cs, const bool* deleted)
{
	if (cnf.empty()) return;
	C_REF* j = cnf;
	forall_cnf(cnf, i) {
		C_REF r = *i;
		if (deleted[r]) continue;
		moveClause(r, new_cm, cs);
		*j++ = r; // must follow moveClause
	}
	assert(j >= cnf);
	cnf.resize(uint32(j - cnf));
}

