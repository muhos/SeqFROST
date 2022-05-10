/***********************************************************************[model.hpp]
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

#ifndef __MODEL_
#define __MODEL_

#include "vector.hpp"
#include "definitions.hpp"

namespace SeqFROST {

	struct MODEL {
		Vec<LIT_ST> value, marks;
		uVec1D lits, resolved;
		LIT_ST* orgvalues;
		uint32 *vorg, maxVar, orgVars, orgClauses, orgLiterals;
		bool extended, verified;
		MODEL() :
			orgvalues(NULL)
			, vorg(NULL), maxVar(0), orgVars(0), orgClauses(0), orgLiterals(0)
			, extended(false), verified(true)
		{}
		~MODEL() {
			maxVar = 0;
			extended = false;
			verified = true;
			lits.clear(true);
			marks.clear(true);
			value.clear(true);
			resolved.clear(true);
		}
		void			init			(uint32*);
		void			print			();
		void			printResolved	();
		void			printValues		();
		void			printClause		(const Lits_t&, const bool&);
		void			extend			(LIT_ST*);
		void			verify			(const string&);
		bool			verify			(char*& clause);
		inline uint32	size			()					const { return extended ? value.size() - 1 : 0; }
		inline int		lit2int			(const uint32& lit) const { return SIGN(lit) ? -int(ABS(lit)) : int(ABS(lit)); }
		inline LIT_ST	operator[]		(const uint32& v)	const { assert(v && v < value.size()); return value[v]; }
		inline LIT_ST&	operator[]		(const uint32& v)		  { assert(v && v < value.size()); return value[v]; }
		inline bool		satisfied		(const uint32& orglit) const
		{
			assert(orglit > 1);
			assert(ABS(orglit) < value.size());
			return value[ABS(orglit)] == !SIGN(orglit);
		}
		inline void		saveLiteral		(const uint32& lit)
		{
			CHECKLIT(lit);
			assert(vorg);
			assert(V2DEC(vorg[ABS(lit)], SIGN(lit)) > 1);
			resolved.push(V2DEC(vorg[ABS(lit)], SIGN(lit)));
		}
		inline void		saveWitness		(const uint32& witness)
		{
			saveLiteral(witness);
			resolved.push(1);
		}
		inline void		saveBinary		(const uint32& witness, const uint32& other)
		{
			CHECKLIT(witness);
			CHECKLIT(other);
			saveLiteral(witness);
			saveLiteral(other);
			resolved.push(2);
			saveWitness(FLIP(witness));
		}
		inline void		saveClause		(const uint32* lits, const int& size, const uint32& witlit)
		{
			assert(size > 1);
			CHECKLIT(witlit);
			const uint32 last = resolved.size();
			int pos = -1;
			for (int i = 0; i < size; ++i) {
				const uint32 lit = lits[i];
				saveLiteral(lit);
				if (lit == witlit)
					pos = i;
			}
			assert(pos >= 0);
			if (pos)
				std::swap(resolved[pos + last], resolved[last]);
			resolved.push(size);
		}

		#define	breakmodel(X, LEN) if (X > 1 && X < maxVar - 2 && X % LEN == 0) { PRINT("\nv "); }

	};

}

#endif