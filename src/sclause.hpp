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

#ifndef __SCLAUSE_
#define __SCLAUSE_

#include "clause.hpp"

namespace SeqFROST {

	/*****************************************************/
	/*  Usage:    abstract clause for simp. on host      */
	/*  Dependency:  none                                */
	/*****************************************************/

	#if defined(_WIN32)
	#pragma warning(push)
	#pragma warning(disable : 4200)
	#endif

	#if defined(__linux__) || defined(__CYGWIN__)
	#define __forceinline __attribute__((always_inline))
	#endif

	#define finline __forceinline 


	class SCLAUSE {

		uint32 _st : 2, _f : 1, _a : 1, _u : 2;
		uint32 _lbd : 26;
		uint32 _sig;
		int _sz;
		uint32 _lits[];

	public:
		finline			SCLAUSE		() :
			_st(ORIGINAL)
			, _f(0)
			, _a(0)
			, _u(0)
			, _lbd(0)
			, _sig(0)
			, _sz(0)
		{}
		finline			SCLAUSE		(const CLAUSE& src) :
			_st(src.learnt())
			, _f(0)
			, _a(0)
			, _sig(0)
			, _sz(src.size())
		{ 
			assert(original() == !src.learnt());
			if (learnt()) {
				_lbd = src.lbd() & MAX_LBD_M;
				_u = src.usage();
			}
			else { _lbd = 0, _u = 0; }
			copyLits(src);
		}
		finline			SCLAUSE     (const SCLAUSE& src) :
			_st(src.status())
			, _f(src.molten())
			, _a(src.added())
			, _u(src.usage())
			, _lbd(src.lbd())
			, _sig(src.sig())
			, _sz(src.size())
		{
			assert(_lbd <= MAX_LBD);
			assert(!src.deleted());
			copyLits(src);
		}
		finline			SCLAUSE		(const Lits_t& src) :
			_st(ORIGINAL)
			, _f(0)
			, _a(0)
			, _u(0)
			, _lbd(0)
			, _sig(0)
			, _sz(src.size())
		{
			copyLits(src);
		}
		template <class SRC>
		finline void	copyLits(const SRC& src) {
			assert(_sz > 0 && _sz <= src.size());
			for (int k = 0; k < _sz; ++k) {
				CHECKLIT(src[k]);
				_lits[k] = src[k];
			}
		}
		finline void	copyLitsFrom(const Lits_t& src) {
			const int size = src.size();
			assert(_sz > 0 && _sz >= size);
			for (int k = 0; k < size; ++k) {
				CHECKLIT(src[k]);
				_lits[k] = src[k];
			}
		}
		finline void	set_lbd		(const uint32& lbd)	{ assert(_lbd < MAX_LBD); _lbd = lbd; }
		finline void	set_sig		(const uint32& sig)		{ _sig = sig; }
		finline void	set_usage	(const CL_ST& usage)	{ _u = usage; }
		finline void	set_status	(const CL_ST& status)	{ _st = status; }
		finline void	resize		(const int& n)		 { _sz = n; }
		finline void	shrink		(const int& n)		 { assert(_sz >= n); _sz -= n; }
		finline uint32&	operator [] (const int& i)		 { assert(i < _sz); return _lits[i]; }
		finline uint32	operator [] (const int& i) const { assert(i < _sz); return _lits[i]; }
		finline operator uint32*	()		 { assert(_sz >= 0); return _lits; }
		finline uint32*	data		()		 { return _lits; }
		finline uint32*	end			()		 { return _lits + _sz; }
		finline void	pop			()		 { assert(_sz > 0); _sz--; }
		finline void	freeze		()		 { _f = 0; }
		finline void	melt		()		 { _f = 1; }
		finline void	markAdded	()		 { _a = 1; }
		finline void	markDeleted	()		 { _st = DELETED; }
		finline uint32	back        ()		 { assert(_sz >= 0); return _lits[_sz - 1]; }
		finline uint32	back        () const { assert(_sz >= 0); return _lits[_sz - 1]; }
		finline CL_ST	usage		() const { return _u; }
		finline bool	molten		() const { return _f; }
		finline bool	added		() const { return _a; }
		finline bool	empty		() const { return !_sz; }
		finline bool	original	() const { return !_st; }
		finline bool	deleted		() const { return _st & DELETED; }
		finline bool	learnt		() const { return _st & LEARNT; }
		finline CL_ST	status		() const { return _st; }
		finline int		size		() const { return _sz; }
		finline uint32  lbd			() const { return _lbd; }
		finline uint32	sig			() const { return _sig; }
		finline int		hasZero		() {
			for (int i = 0; i < _sz; ++i)
				if (!_lits[i]) 
					return i;
			return -1;
		}
		finline bool	isSorted	() {
			for (int i = 0; i < _sz; ++i) {
				if (i > 0 && _lits[i] < _lits[i - 1]) 
					return false;
			}
			return true;
		}
		finline void	calcSig		(const uint32& init_sig = 0) {
			_sig = init_sig;
			for (int i = 0; i < _sz; ++i)
				_sig |= MAPHASH(_lits[i]);
		}
		finline bool	has			(const uint32& lit) {
			if (_sz == 2) {
				if (_lits[0] == lit || _lits[1] == lit) return true;
				else return false;
			}
			else {
				assert(this->isSorted());
				int low = 0, high = _sz - 1, mid;
				uint32 first = _lits[low], last = _lits[high];
				while (first <= lit && last >= lit) {
					mid = (low + high) >> 1;
					uint32 m = _lits[mid];
					if (m < lit) first = _lits[low = mid + 1];
					else if (m > lit) last = _lits[high = mid - 1];
					else return true; // found
				}
				if (_lits[low] == lit) return true; // found
				else return false; // Not found
			}
		}
		finline void	print		() const {
			printf("(");
			for (int l = 0; l < _sz; ++l) {
				int lit = int(ABS(_lits[l]));
				lit = (SIGN(_lits[l])) ? -lit : lit;
				printf("%4d ", lit);
			}
			char st = 'U';
			if (deleted()) st = 'X';
			else if (added()) st = 'A';
			else if (original()) st = 'O';
			else if (learnt()) st = 'L';
			printf(") %c:%d, used=%d, lbd=%d, s=0x%X\n", st, molten(), usage(), _lbd, _sig);
		}
	};

	constexpr size_t SIMP_BUCKETSIZE = sizeof(uint32);
	constexpr size_t SIMP_CLAUSESIZE = sizeof(SCLAUSE);
	constexpr size_t SIMP_CLAUSEBUCKETS = SIMP_CLAUSESIZE / SIMP_BUCKETSIZE;

	#define SCBUCKETS(CLAUSESIZE) \
		(SIMP_CLAUSEBUCKETS + (CLAUSESIZE))

	#define REGIONBUCKETS(NCLS,NLITS) \
		((NCLS) * SIMP_CLAUSEBUCKETS + (NLITS))

	#define mark_ssubsume(C) \
	{ \
		assert(C.added()); \
		forall_clause(C, k) { \
			sp->state[ABS(*k)].subsume = 1; \
		} \
	}

	#if defined(_WIN32)
	#pragma warning(pop)
	#endif

}

#endif
