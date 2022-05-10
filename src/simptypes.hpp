/***********************************************************************[simptypes.hpp]
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

#ifndef __SIMP_TYPES_
#define __SIMP_TYPES_

#include "memory.hpp"
#include "datatypes.hpp"
#include "vector.hpp"
#include "sclause.hpp"

namespace SeqFROST {

	constexpr int	 MAXFUNVAR = 12;
	constexpr uint32 FUNTABLEN = 64;  // (1 << (12 - 6))

	extern uint32 orgcore[MAXFUNVAR];

	const uint64 magicnumbers[6] = {
		0xaaaaaaaaaaaaaaaaull, 0xccccccccccccccccull, 0xf0f0f0f0f0f0f0f0ull,
		0xff00ff00ff00ff00ull, 0xffff0000ffff0000ull, 0xffffffff00000000ull,
	};

	struct OCCUR { uint32 ps, ns; };

	typedef uint64				Fun[FUNTABLEN];
	typedef uint64				S_REF;
	typedef Vec<S_REF, int>		OL;
	typedef Vec<OL>				OT;
	typedef SMM<uint32, S_REF>	STYPE;
	typedef Vec<S_REF>			cnf_refs_t;

	class SCNF : public STYPE {

		cnf_refs_t _refs;

	public:
		inline						SCNF		() { 
			assert(STYPE::bucket() == SIMP_BUCKETSIZE);
            assert(SOLVER_LITSIZE == sizeof(uint32));
            assert(SIMP_CLAUSESIZE == sizeof(SCLAUSE)); 
			assert(SIMP_CLAUSEBUCKETS == SIMP_CLAUSESIZE / SIMP_BUCKETSIZE); 
		}
		inline		 void			init        (const S_REF& nCls, const S_REF& nLits) {
			STYPE::init(REGIONBUCKETS(nCls, nLits));
			_refs.reserve((uint32)nCls);
		}
		inline		 void			destroy		()		 { STYPE::dealloc(), _refs.clear(true); }
		inline 		 cnf_refs_t&	refs		()		 { return _refs; }
		inline		 uint32			size		() const { return _refs.size(); }
		inline		 bool			empty		() const { return _refs.empty(); }
		inline 		 S_REF*			end			()		 { return _refs.end(); }
		inline		 S_REF			ref			(const uint32& i)		{ assert(i < _refs.size()); return _refs[i]; }
		inline const S_REF&			ref			(const uint32& i) const { assert(i < _refs.size()); return _refs[i]; }
		inline		 SCLAUSE*		clause		(const S_REF& r)		{ return (SCLAUSE*)STYPE::address(r); }
		inline const SCLAUSE*		clause		(const S_REF& r)  const { return (SCLAUSE*)STYPE::address(r); }
		inline		 SCLAUSE&		operator[]	(const S_REF& r)		{ return (SCLAUSE&)STYPE::operator[](r); }
		inline const SCLAUSE&		operator[]	(const S_REF& r)  const { return (SCLAUSE&)STYPE::operator[](r); }
		template <class SRC>
		inline		 SCLAUSE*		alloc		(const SRC& src) {
			assert(src.size());
			const S_REF r = STYPE::alloc(SCBUCKETS(src.size()));
            SCLAUSE* c = new (clause(r)) SCLAUSE(src);
			_refs.push(r);
			return c;
		}
		inline		 S_REF*			alloc		(S_REF& r, const int& nCls, const int& nLits) {
			assert(nLits >= nCls);
			r = STYPE::alloc(REGIONBUCKETS(nCls, nLits));
			const uint32 currSize = _refs.size();
			_refs.expand(nCls + currSize);
			return _refs.data() + currSize;
		}
		inline		 void			migrateTo   (SCNF& dest) {
            STYPE::migrateTo(dest);
            _refs.migrateTo(dest.refs());
        }
		inline		 void			print		(const uint32& off = 0, const bool& p_ref = true) {
			for (uint32 i = off; i < size(); ++i) {
				const S_REF r = ref(i);
				const SCLAUSE& c = operator[](r);
				if (c.size()) {
					if (p_ref) printf("c  C(%d, r: %lld)->", i, _refs[i]);
					else printf("c  C(%d)->", i);
					c.print();
				}
			}
		}
		inline		 void			printAdded	(const uint32& off = 0, const bool& p_ref = true) {
			for (uint32 i = off; i < size(); ++i) {
				const S_REF r = ref(i);
				const SCLAUSE& c = operator[](r);
				if (c.size() && c.added()) {
					if (p_ref) printf("c  C(%d, r: %lld)->", i, _refs[i]);
					else printf("c  C(%d)->", i);
					c.print();
				}
			}
		}
		inline		 void			printDeleted(const uint32& off = 0, const bool& p_ref = true) {
			for (uint32 i = off; i < size(); ++i) {
				const S_REF r = ref(i);
				const SCLAUSE& c = operator[](r);
				if (c.size() && c.deleted()) {
					if (p_ref) printf("c  C(%d, r: %lld)->", i, _refs[i]);
					else printf("c  C(%d)->", i);
					c.print();
				}
			}
		}
	};

	#define forall_occurs(LIST, PTR) \
		for (S_REF* PTR = LIST, *END = LIST.end(); PTR != END; ++PTR)

	#define forall_sclauses(SCNF, PTR) \
		for (S_REF* PTR = SCNF.refs(), *END = SCNF.end(); PTR != END; ++PTR)

}

#endif