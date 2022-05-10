/***********************************************************************[clause.hpp]
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

#ifndef __CLAUSE_
#define __CLAUSE_

#include "datatypes.hpp"
#include "vector.hpp"
#include <cassert>

namespace SeqFROST {

	/*****************************************************/
	/*  Usage:   structure for CNF clause storage        */
	/*  Dependency:  vector, uint32                      */
	/*****************************************************/

	#if defined(_WIN32)
	#pragma warning(push)
	#pragma warning(disable : 26495)
	#elif defined(__linux__) || defined(__CYGWIN__)

	#define __forceinline __attribute__((always_inline))

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Warray-bounds"
	#endif

	#define finline __forceinline 


	class CLAUSE {

		bool	_r : 1;
		bool	_m : 1;
		bool	_s : 1;
		bool	_h : 1;
		bool	_k : 1;
		bool	_b : 1;
		bool	_l;
		bool	_v;
		CL_ST	_used; 
		int		_sz, _pos, _lbd;
		union { uint32 _lits[2]; C_REF _ref; };

	public:
		size_t capacity() const { return ((size_t(_sz) - 2) * sizeof(uint32) + sizeof(*this)) / sizeof(uint32); }
		finline CLAUSE() :
			  _r(false)
			, _m(false)
			, _s(false)
			, _h(false)
			, _k(true)
			, _b(false)
			, _l(false)
			, _v(false)
			, _used(0)
			, _sz(0)
			, _pos(2)
			, _lbd(0)
		{ }
		finline CLAUSE(const int& size) : 
			  _r(false)
			, _m(false)
			, _s(false)
			, _h(false)
			, _k(true)
			, _l(false)
			, _v(false)
			, _used(0)
			, _sz(size)
			, _pos(2)
			, _lbd(0)
		{ 
			assert(_sz > 1);
			_b = _sz == 2;
		}
		finline CLAUSE(const Lits_t& lits) : 
			  _r(false)
			, _m(false)
			, _s(false)
			, _h(false)
			, _k(true)
			, _l(false)
			, _v(false)
			, _used(0)
			, _sz(lits.size())
			, _pos(2)
			, _lbd(0)
		{ 
			assert(_sz > 1);
			_b = _sz == 2; 
			copyLits(lits);
		}
		finline CLAUSE(const CLAUSE& src) : 
			  _r(src.reason())
			, _m(false)
			, _s(src.subsume())
			, _h(src.hyper())
			, _k(src.keep())
			, _l(src.learnt())
			, _v(src.vivify())
			, _used(src.usage())
			, _sz(src.size())
			, _pos(src.pos())
			, _lbd(src.lbd())
		{ 
			assert(_sz > 1);
			_b = _sz == 2;
			copyLits(src);
		}
		template <class SRC>
		finline	void	 copyLits(const SRC& src) {
			assert(_sz > 1);
			for (int k = 0; k < _sz; ++k) {
				assert(src[k] > 1);
				_lits[k] = src[k];
			}
		}
		finline	uint32&	 operator[]	(const int& i) { assert(i < _sz); return _lits[i]; }
		finline	uint32	 operator[]	(const int& i) const { assert(i < _sz); return _lits[i]; }
		finline	operator uint32*	() { assert(_sz); return _lits; }
		finline	uint32*	data		() { return _lits; }
		finline	uint32*	mid			() { assert(_pos < _sz); return _lits + _pos; }
		finline	uint32*	end			() { return _lits + _sz; }
		finline	void	pop			() { assert(_sz); _sz--, _b = _sz == 2; }
		finline	C_REF	ref			() const { assert(_m); return _ref; }
		finline	int		pos			() const { assert(_pos > 1); return _pos; }
		finline	CL_ST	usage		() const { assert(_used < USAGET1); return _used; }
		finline	int		size		() const { return _sz; }
		finline	int		lbd			() const { return _lbd; }
		finline bool	empty		() const { return !_sz; }
		finline	bool	original	() const { return !_l; }
		finline	bool	learnt		() const { return _l; }
		finline	bool	reason		() const { return _r; }
		finline	bool	moved		() const { return _m; }
		finline	bool	subsume		() const { return _s; }
		finline	bool	vivify		() const { return _v; }
		finline	bool	keep		() const { return _k; }
		finline	bool	hyper		() const { return _h; }
		finline	bool	binary		() const { assert(_sz > 2 || _b); return _b; }
		finline	void	initTier2	() { _used = USAGET2; }
		finline	void	initTier3	() { _used = USAGET3; }
		finline	void	warm		() { assert(_used); _used--; }
		finline void	initMoved	() { _m = false; }
		finline	void	initReason	() { _r = false; }
		finline	void	initSubsume	() { _s = false; }
		finline	void	initVivify	() { _v = false; }
		finline	void	markOriginal() { _l = false; }
		finline	void	markLearnt	() { _l = true; }
		finline	void	markReason	() { _r = true; }
		finline	void	markMoved	() { _m = true; }
		finline	void	markHyper	() { _h = true; }
		finline	void	markSubsume	() { _s = true; }
		finline	void	markVivify	() { _v = true; }
		finline	void	shrink		(const int& n) { 
			_sz -= n; 
			if (_pos >= _sz) _pos = 2;
			_b = _sz == 2;
		}
		finline	void	resize		(const int& newSz) {
			_sz = newSz;
			if (_pos >= _sz) _pos = 2;
			_b = _sz == 2;
		}
		finline	void	set_ref		(const C_REF& r) { _m = 1, _ref = r; }
		finline	void	set_pos		(const int& newPos) { assert(newPos >= 2); _pos = newPos; }
		finline	void	set_lbd		(const int& lbd) { _lbd = lbd; }
		finline	void	set_usage	(const CL_ST& usage) { _used = usage; }
		finline	void	set_keep	(const bool& keep) { _k = keep; }
		finline	void	print		() const {
			PRINT("(");
			for (int l = 0; l < _sz; ++l) {
				int lit = int(ABS(_lits[l]));
				lit = (SIGN(_lits[l])) ? -lit : lit;
				PRINT("%4d ", lit);
			}
			char st = 'U';
			if (original()) st = 'O';
			else if (learnt()) st = 'L';
			PRINT(") %c:%d[u:%d k:%d h:%d lbd:%d]\n", 
				st, reason(), usage(), keep(), hyper(), lbd());
		}

	};

#if defined(_WIN32)
#pragma warning(pop)
#elif defined(__linux__) || defined(__CYGWIN__)
#pragma GCC diagnostic pop
#endif

	constexpr size_t SOLVER_LITSIZE = sizeof(uint32);
	constexpr size_t SOLVER_CLAUSESIZE = sizeof(CLAUSE);
	constexpr size_t SOLVER_CLAUSEBUCKETS = (SOLVER_CLAUSESIZE / SOLVER_LITSIZE) - 2;

	#define CBUCKETS(CLAUSESIZE) \
			(SOLVER_CLAUSEBUCKETS + (CLAUSESIZE))

	#define INITNBUCKETS(NCLS,NLITS) \
			((NCLS) * SOLVER_CLAUSEBUCKETS + (NLITS))

	#define GET_CLAUSE(C,REF,CS) CLAUSE& C = (CLAUSE&)CS[REF];	

	#define GET_CLAUSE_PTR(C,REF,CS) CLAUSE* C = (CLAUSE*)(CS + REF);	

	#define keeping(C) (C.original() || C.keep() || (C.lbd() <= limit.keptlbd && C.size() <= limit.keptsize))

	#define mark_literals(C) \
	{ \
		assert(C.size() > 1); \
		forall_clause(C, k) { \
			assert(UNASSIGNED(sp->marks[ABS(*k)])); \
			const uint32 lit = *k; \
			sp->marks[ABS(lit)] = SIGN(lit); \
		} \
	}

	#define unmark_literals(C) \
	{ \
		forall_clause(C, k) { \
			assert(!UNASSIGNED(sp->marks[ABS(*k)])); \
			sp->marks[ABS(*k)] = UNDEF_VAL; \
		} \
	}

	#define mark_subsume(C) \
	{ \
		assert(keeping(C)); \
		forall_clause(C, k) { \
			sp->state[ABS(*k)].subsume = 1; \
		} \
	}

}

#endif