/***********************************************************************[map.hpp]
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

#ifndef __MAP_
#define __MAP_

#include "solvetypes.hpp"

namespace SeqFROST {

	class MAP {
		SP* sp;
		uVec1D _mapped;
		uint32 newVars, firstDL0, mappedFirstDL0;
		LIT_ST valFirstDL0;

		#define MAPVAR(OLD) _mapped[OLD] = ++newVars

		#define MAPPED(OLD) _mapped[OLD]

	public:
							~MAP				() { destroy(); }
							MAP					() : 
								sp(NULL)
								, newVars(0)
								, firstDL0(0)
								, mappedFirstDL0(0)
								, valFirstDL0(UNDEF_VAL) 
							{}

		inline uint32*		operator*			() { return _mapped; }
		inline bool			empty				() const { return !newVars; }
		inline uint32		size				() const { return newVars + 1; }
		inline uint32		numVars				() const { return newVars; }
		inline uint32		firstL0				() const { return firstDL0; }
		inline uint32		mapped				(const uint32& old) const { return _mapped[old]; }
		inline uint32		mapLit				(uint32 lit) 
		{
			assert(!_mapped.empty());

			const uint32 oldVar = ABS(lit);
			const uint32 newVar = MAPPED(oldVar);

			assert(newVar <= newVars);

			if (newVar && NEQUAL(oldVar, firstDL0)) {
				assert(UNASSIGNED(sp->value[lit]));
				assert(!sp->state[oldVar].state);
				return V2DEC(newVar, SIGN(lit));
			}

			const LIT_ST val = sp->value[lit];

			if (UNASSIGNED(val)) 
				return 0;

			assert(val >= 0);

			lit = V2L(mappedFirstDL0);

			return (valFirstDL0 == val) ? lit : FLIP(lit);
		}
		inline void			initiate			(SP* _sp) {
			assert(inf.maxVar);
			assert(_sp != NULL);
			sp = _sp;
			_mapped.resize(inf.maxVar + 1, 0);
			const LIT_ST* values = sp->value;
			forall_variables(old) {
				if (!sp->state[old].state) MAPVAR(old);
				else if (!firstDL0 && !UNASSIGNED(values[V2L(old)])) {
					firstDL0 = old, valFirstDL0 = values[V2L(firstDL0)];
					MAPVAR(firstDL0);
					mappedFirstDL0 = newVars;
				}
			}
			assert(newVars <= inf.maxVar);
			LOG2(2, " Mapped %d to %d, first frozen/autartic literal \"%d\"", inf.maxVar, newVars,
				firstDL0 ? (valFirstDL0 ? firstDL0 : -int(firstDL0)) : 0);
		}
		inline void			mapTransitive		(uint32& lit) {
			if (lit <= 2) return;
			CHECKLIT(lit);
			uint32 v = ABS(lit);
			uint32 mlit = 0;
			if (sp->state[v].state) {
				while (v <= inf.maxVar && sp->state[v].state) v++;
				if (v <= inf.maxVar) mlit = mapLit(V2L(v));
			}
			else mlit = mapLit(lit);
			if (!mlit) mlit = 2;
			lit = mlit;
		}
		inline void			mapSP				(SP* to) {
			// map all arrays
			forall_variables(v) {
				const uint32 mVar = MAPPED(v);
				if (mVar) {
					const uint32 p = V2L(v), n = NEG(p);
					const uint32 mpos = V2L(mVar), mneg = NEG(mpos);
					// map 'value'
					to->value[mpos] = sp->value[p];
					to->value[mneg] = sp->value[n];
					// map others
					to->source[mVar] = sp->source[v];
					to->level[mVar] = sp->level[v];
					to->board[mVar] = sp->board[v];
					to->marks[mVar] = sp->marks[v];
					to->state[mVar] = sp->state[v];
					// map phases
					const Phase_t& phsrc = sp->phase[v];
					Phase_t& phdest = to->phase[mVar];
					phdest.best = phsrc.best;
					phdest.saved = phsrc.saved;
					phdest.target = phsrc.target;
				}
			}
			// update counters
			to->propagated = sp->propagated;
			to->simplified = firstDL0 ? 1 : 0;
			sp = NULL; // nullify local reference
		}
		inline void			mapOrgs             (Lits_t& lits) {
			forall_vector(uint32, lits, i) {
				const uint32 lit = *i;
				if (lit) {
					CHECKLIT(lit);
					*i = mapLit(lit);
					LOG2(4, " Literal %d mapped to %d", l2i(lit), *i ? l2i(*i) : 0);
				}
			}
		}
		inline void			mapOrgs             (uVec1D& lits) {
			forall_vector(uint32, lits, i) {
				const uint32 lit = *i;
				if (lit) {
					CHECKLIT(lit);
					*i = mapLit(lit);
					LOG2(4, " Literal %d mapped to %d", l2i(lit), *i ? l2i(*i) : 0);
				}
			}
		}
		template <class T>
		inline void			mapShrinkVars		(Vec<T>& vars) {
			assert(inf.maxVar >= newVars);
			forall_variables(v) {
				uint32 mVar = MAPPED(v);
				if (mVar) vars[mVar] = vars[v];
			}
			vars.resize(size());
			vars.shrinkCap();
		}
		template <class T>
		inline void			mapShrinkDualVars	(Vec<T>& vars) {
			assert(inf.maxVar >= newVars);
			forall_variables(v) {
				uint32 mVar = MAPPED(v);
				if (mVar) {
					uint32 p = V2L(v), n = NEG(p);
					uint32 mpos = V2L(mVar), mneg = NEG(mpos);
					vars[mpos] = vars[p];
					vars[mneg] = vars[n];
				}
			}
			vars.resize(V2L(size()));
			vars.shrinkCap();
		}
		inline void			mapShrinkLits		(uVec1D& lits) {
			uint32 *d = lits;
			forall_vector(uint32, lits, s) {
				const uint32 srclit = *s;
				CHECKLIT(srclit);
				uint32 mVar = MAPPED(ABS(srclit));
				if (mVar) *d++ = V2DEC(mVar, SIGN(srclit));
			}
			lits.resize(uint32(d - lits));
			lits.shrinkCap();
		}
		template <class SRC, class DEST>
		inline void			mapClause			(DEST& dest, SRC& src) {
			assert(src.size() > 1);
			assert(!src.deleted());
			LOGCLAUSE(4, src, " Clause    ");
			for (int i = 0; i < src.size(); ++i) {
				CHECKLIT(src[i]);
				assert(UNASSIGNED(sp->value[src[i]]));
				dest[i] = mapLit(src[i]);
			}
			LOGCLAUSE(4, dest, " mapped to ");
		}
		template <class C>
		inline void			mapClause			(C& c) {
			assert(c.size() > 1);
			assert(!c.moved());
			LOGCLAUSE(4, c, " Clause    ");
			forall_clause(c, i) {
				CHECKLIT(*i);
				assert(UNASSIGNED(sp->value[*i]));
				*i = mapLit(*i);
			}
			LOGCLAUSE(4, c, " mapped to ");
		}
		inline void			mapClauses			(CMM& cm, BCNF& cnf)
		{
			if (cnf.empty()) return;
			forall_cnf(cnf, i) {
				mapClause(cm[*i]);
			}
		}
		inline void			mapWatches			(WL& ws)
		{
			if (ws.empty()) return;
			forall_watches(ws, w) {
				w->imp = mapLit(w->imp);
			}
		}
		inline void			mapWatches			(WT& wt)
		{
			if (wt.empty()) return;
			forall_variables(v) {
				const uint32 mVar = MAPPED(v);
				if (mVar) {
					const uint32 mpos = V2L(mVar), mneg = NEG(mpos);
					if (NEQUAL(mVar, v)) { // map watch lists
						const uint32 p = V2L(v), n = NEG(p);
						wt[mpos].moveFrom(wt[p]);
						wt[mneg].moveFrom(wt[n]);
					}
					mapWatches(wt[mpos]), mapWatches(wt[mneg]); // then map watch imps
				}
			}
			wt.resize(V2L(size()));
			wt.shrinkCap();
		}
		inline void			destroy				() {
			sp = NULL, _mapped.clear(true);
			newVars = 0, firstDL0 = mappedFirstDL0 = 0;
			valFirstDL0 = UNDEF_VAL;
		}
	};

}

#endif