/***********************************************************************[space.hpp]
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

#ifndef __SPACE_
#define __SPACE_

#include "state.hpp"
#include "malloc.hpp"
#include "rephase.hpp"
#include "definitions.hpp"


namespace SeqFROST {

	/*****************************************************/
	/*  Usage:    Information of search space            */
	/*  Dependency: none                                 */
	/*****************************************************/

	class SP {

		addr_t		_mem;
		size_t		_sz, _cap;

		template <class T>
		inline size_t calcBytes(const uint32& sz, const uint32& nVecs) const {
			assert(sz); 
			return sz * nVecs * sizeof(T);
		}

		#define forall_space(X) \
			for (uint32 X = 1; X < _sz; ++X)

		#define	breakline(X) \
			if (X > 1 && X < _sz - 2 && X % 10 == 0) { \
				PUTCH('\n'); LOGN0("\t\t"); \
			}

	public:
		// arrays
		uint32	 *level;
		uint32   *board;
		uint32   *trailpos;
		uint32   *tmpstack, *stacktail;
		C_REF	 *source;
		State_t  *state;
		Phase_t	 *phase;
		LIT_ST	 *seen,	*frozen, *marks;
		LIT_ST	 *value;
		
		// scalers
		int learntLBD;
		int reasonsize, resolventsize;
		int conflictdepth, conflictsize;
		uint32 trailpivot;
		uint32 simplified;
		uint32 propagated;
		//================
		SP() { RESETSTRUCT(this); }
		SP(const uint32& size, const LIT_ST& pol) 
		{
			RESETSTRUCT(this);
			assert(pol >= 0 && pol <= UNDEF_PHASE);
			assert(sizeof(C_REF) == sizeof(uint64));
			assert(sizeof(State_t) == sizeof(Byte));
			assert(sizeof(Phase_t) == sizeof(Byte));

			const size_t vec8Bytes = calcBytes<uint64>(size, 1);
			const size_t vec4Bytes = calcBytes<uint32>(size, 4);
			const size_t vec1Bytes = calcBytes<Byte>(size, 7);

			_sz = size;
			_cap = vec1Bytes + vec4Bytes + vec8Bytes;

			assert(!_mem);
			assert(_cap);
			assert(_sz);

			_mem = sfcalloc<Byte>(_cap);

			// 8-byte arrays
			source		= (C_REF*)_mem;

			// 4-byte arrays
			level		= (uint32*)(_mem + vec8Bytes);
			board		= level    + _sz;
			trailpos	= board	   + _sz;
			tmpstack	= trailpos + _sz;

			// 1-byte arrays
			addr_t tmp  = _mem  + vec8Bytes + vec4Bytes;
			value		= (LIT_ST*)tmp;
			marks		= value	 + _sz + _sz;
			frozen		= marks	 + _sz;
			seen		= frozen + _sz;
			state		= (State_t*)(seen  + _sz);
			phase		= (Phase_t*)(state + _sz);

			assert(_mem + _cap == addr_t(phase) + _sz);

			// initialize value and marks 
			memset(tmp, UNDEF_VAL, _sz + _sz + _sz);

			// initialize others
			forall_space(v) {
				level[v] = UNDEF_LEVEL;
				source[v] = UNDEF_REF;
				Phase_t& ph = phase[v];
				ph.best = UNDEF_PHASE;
				ph.target = UNDEF_PHASE;
				ph.saved = pol;
			}
		}
		size_t	size		() const { return _sz; }
		size_t	capacity	() const { return _cap; }
		void	resetTarget () {
			forall_space(v) {
				phase[v].target = UNDEF_PHASE;
			}
		}
		void	saveToTarget() {
			forall_space(v) {
				Phase_t& ph = phase[v];
				ph.target = ph.saved;
			}
		}
		void	saveToBest	() {
			forall_space(v) {
				Phase_t& ph = phase[v];
				ph.best = ph.saved;
			}
		}
		void	copyFrom	(SP* src)
		{
			propagated = src->propagated;
			trailpivot = src->trailpivot;
			simplified = src->simplified;
			forall_space(v) {
				const uint32 p = V2L(v), n = NEG(p);
				value[p] = src->value[p];
				value[n] = src->value[n];
				level[v] = src->level[v];
				state[v] = src->state[v];
			}
		}
		void	printStates	() {
			LOGN1(" States->[");
			forall_space(v) {
				PRINT("%5d:%d ", v, state[v].state);
				breakline(v);
			}
			putc(']', stdout), PUTCH('\n');
		}
		void	printValues	() {
			LOGN1(" Values->[");
			forall_space(v) {
				uint32 lit = V2L(v);
				PRINT("%5d:%d ", l2i(lit), value[lit]);
				breakline(v);
			}
			putc(']', stdout), PUTCH('\n');
		}
		void	printLevels	() {
			LOGN1(" Levels->[");
			forall_space(v) {
				PRINT("%5d@%d ", v, level[v]);
				breakline(v);
			}
			putc(']', stdout), PUTCH('\n');
		}
		void	clearBoard	() { memset(board, 0, sizeof(uint32) * _sz); }
		void	clearSubsume() { forall_space(v) state[v].subsume = 0; }
		void	destroy		() { if (_mem) std::free(_mem); }
				~SP			() { destroy(); }
	};

}

#endif