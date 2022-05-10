/***********************************************************************[watch.cpp]
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

void Solver::attachBins(BCNF& cnf, const cbucket_t* cs, const bool* deleted, const bool& hasElim)
{
    assert(!wt.empty());
    if (hasElim) {
        const State_t* states = sp->state;
        forall_cnf(cnf, i) {
            const C_REF r = *i;
            if (deleted[r]) continue;
            assert(r < cm.size());
            GET_CLAUSE(c, r, cs);
            if (c.binary()) {
                if (MELTED(states[ABS(c[0])].state)
                    || MELTED(states[ABS(c[1])].state)) {
                    removeClause(c, r);
                }
                else 
                    ATTACH_TWO_WATCHES(r, c);
            }
        }
    }
    else {
        forall_cnf(cnf, i) {
            const C_REF r = *i;
            if (deleted[r]) continue;
            assert(r < cm.size());
            GET_CLAUSE(c, r, cs);
            if (c.binary()) 
                ATTACH_TWO_WATCHES(r, c);
        }
    }
}

void Solver::attachNonBins(BCNF& cnf, const cbucket_t* cs, const bool* deleted, const bool& hasElim)
{
    assert(!wt.empty());
    if (hasElim) {
        const State_t* states = sp->state;
        forall_cnf(cnf, i) {
            const C_REF r = *i;
            if (deleted[r]) continue;
            assert(r < cm.size());
            GET_CLAUSE(c, r, cs);
            if (c.binary()) continue;
            bool removed = false;
            forall_clause(c, k) {
                const uint32 lit = *k;
                if (MELTED(states[ABS(lit)].state)) {
                    removed = true;
                    break;
                }
            }
			if (removed)
				removeClause(c, r);
            else {
                sortClause(c);
                ATTACH_TWO_WATCHES(r, c);
            }
        }
    }
    else {
        forall_cnf(cnf, i) {
            const C_REF r = *i;
            if (deleted[r]) continue;
            assert(r < cm.size());
            GET_CLAUSE(c, r, cs);
            if (c.binary()) continue;
            sortClause(c);
            ATTACH_TWO_WATCHES(r, c);
        }
    }
}

void Solver::attachClauses(BCNF& cnf, const cbucket_t* cs, const bool* deleted, const bool& hasElim)
{
    assert(!wt.empty());
    if (hasElim) {
        const State_t* states = sp->state;
        forall_cnf(cnf, i) {
            const C_REF r = *i;
            if (deleted[r]) continue;
            assert(r < cm.size());
            GET_CLAUSE(c, r, cs);
            bool removed = false;
            forall_clause(c, k) {
                const uint32 lit = *k;
                if (MELTED(states[ABS(lit)].state)) {
                    removed = true;
                    break;
                }
            }
            if (removed)
                removeClause(c, r);
            else {
                if (!c.binary()) 
                    sortClause(c);
                ATTACH_TWO_WATCHES(r, c);
            }
        }
    }
    else {
        forall_cnf(cnf, i) {
            const C_REF r = *i;
            if (deleted[r]) continue;
            assert(r < cm.size());
            GET_CLAUSE(c, r, cs);
            if (!c.binary()) 
                sortClause(c);
            ATTACH_TWO_WATCHES(r, c);
        }
    }
}

void Solver::rebuildWT(const CL_ST& code)
{
    wt.resize(inf.nDualVars);
    PREFETCH_CM(cs, deleted);
    if (PRIORALLBINS(code)) {
        if (PRIORLEARNTBINS(code)) {
            attachBins(learnts, cs, deleted);
            attachBins(orgs, cs, deleted);
        }
        else {
            attachBins(orgs, cs, deleted);
            attachBins(learnts, cs, deleted);
        }
        attachNonBins(orgs, cs, deleted);
        attachNonBins(learnts, cs, deleted);
    }
    else {
        attachClauses(orgs, cs, deleted);
        attachClauses(learnts, cs, deleted);
    }
}

void Solver::sortWT()
{
    WL saved;
    forall_literals(lit) {
        assert(saved.empty());
        WL& ws = wt[lit];
        WATCH *j = ws;
        forall_watches(ws, i) {
            const WATCH w = *i;
            if (w.binary())
                *j++ = w;
            else 
                saved.push(w);
        }
        ws.resize(uint32(j - ws));
        forall_watches(saved, i) { 
            ws.push(*i);
        }
        saved.clear();
    }
    saved.clear(true);
}

void Solver::detachClauses(const bool& keepbinaries)
{
    forall_literals(lit) {
        WL& ws = wt[lit];
        WATCH* j = ws;
        forall_watches(ws, i) {
            const WATCH w = *i;
            if (keepbinaries && w.binary()) 
                *j++ = w;
        }
        ws.resize(uint32(j - ws));
    }
}

void Solver::binarizeWT(const bool& keeplearnts)
{
    assert(!LEVEL);
    const LIT_ST* values = sp->value;
    PREFETCH_CM(cs, deleted);
    forall_literals(lit) {
		const LIT_ST litval = values[lit];
		WL& ws = wt[FLIP(lit)];
		WATCH* j = ws;
		forall_watches(ws, i) {
			const WATCH w = *i;
			if (w.binary()) {
                const C_REF ref = w.ref;
                if (deleted[ref]) continue;
				const uint32 imp = w.imp;
                if (litval > 0 || values[imp] > 0) {
                    if (lit < imp) {
                        GET_CLAUSE(c, ref, cs);
                        removeClause(c, ref);
                    }
                }
                else if (keeplearnts)
                    *j++ = w;  
                else {
                    GET_CLAUSE(c, ref, cs);
                    if (c.original())
                        *j++ = w;                
                }
			}
		}
		ws.resize(uint32(j - ws));
    }
}
