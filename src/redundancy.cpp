/***********************************************************************[elimination.cpp]
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

#include "redundancy.hpp"
#include "subsume.hpp"
#include "sort.hpp"

using namespace SeqFROST;


void Solver::ERE()
{
	if (!opts.ere_en) return;
	if (INTERRUPTED) killSolver();
	LOG2(2, " Eliminating redundances..");
	if (opts.profile_simplifier) timer.pstart();
	const int ereextend = opts.ere_extend;
	const int maxoccurs = opts.ere_max_occurs;
	const int maxsize = opts.ere_clause_max;
#ifdef STATISTICS
	ERESTATS& erestats = stats.simplify.ere;
#endif
	Lits_t merged;
	merged.reserve(maxsize);
	forall_vector(uint32, elected, e) {
		const uint32 x = *e;
		CHECKVAR(x);
		uint32 dx = V2L(x), fx = NEG(dx);

		if (ot[dx].size() > ot[fx].size()) std::swap(dx, fx);

		OL& me = ot[dx], &other = ot[fx];
		const int ds = me.size(), fs = other.size();
		// avoid empty lists and respect maximum length
		if (ds && fs && ds <= maxoccurs && fs <= maxoccurs) {
			// first clause in sorted lists is already larger than the max.
			if (scnf[*me].size() > maxsize || scnf[*other].size() > maxsize)
				continue;
			// do merging and apply forward subsumption
			// check (on-the-fly) over resolvents			
			forall_occurs(me, i) {
				const S_REF iref = *i;
				SCLAUSE& ci = scnf[iref];
				if (ci.deleted()) continue;
				const bool ciorg = ci.original();				
				forall_occurs(other, j) {
					const S_REF jref = *j;
					SCLAUSE& cj = scnf[jref];
					if (cj.deleted()) continue;
					const bool cjorg = cj.original();
					// merge
					uint32 sig, best;
					int len = merge_ere(x, ci, cj, ot, maxsize, merged, sig, best);
                    assert(len <= maxsize);
					if (len < 2) continue;                                 
                    // forward check
                #ifdef STATISTICS
                    erestats.tried++;
                #endif
                    CHECKLIT(best);
                    OL& minlist = ot[best];
					assert(minlist.size());
                    forall_occurs(minlist, m) {
                        const S_REF mref = *m;
                        if (mref == iref || mref == jref) 
							continue;
                        SCLAUSE& c = scnf[mref];
                        if (c.deleted()) continue;
                        const int csize = c.size();
                        if (len <= csize && sub(sig, c.sig()) && sub(merged, c)) {
							const bool learnt = c.learnt();
							const bool equal = len == csize;
                            if (equal && (learnt || (ciorg && cjorg))) { // can be removed
                                LOGCLAUSE(4, c, " Redundant");	
                            #ifdef STATISTICS
                                if (learnt) erestats.learntremoved++;
                                else erestats.orgremoved++;
                            #endif
                                removeClause(c);
								LOGCLAUSE(4, c, " By merged");	
                                break;
                            }
                            else if (ereextend) { // can be strengthened
								if (learnt && ereextend == 1) continue;
                                LOGCLAUSE(4, c, " Strengthened");	
							#ifdef STATISTICS
								if (learnt) erestats.learntstrengthened++;
								else erestats.orgstrengthened++;
							#endif
								assert(merged.size() == len);
								if (opts.proof_en) {
									proof.addClause(merged);							
									proof.deleteClause(c);
								}
								c.copyLitsFrom(merged);
								c.resize(len);
								if (learnt)
									bumpShrunken(c);
                                LOGCLAUSE(4, c, " By merged   ");
                            }
                        }
                    }		
				}
			}
		}
	}
	if (opts.profile_simplifier) timer.pstop(), timer.ere += timer.pcpuTime();
	LOGREDCL(this, 2, "ERE Reductions");
}