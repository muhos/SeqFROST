/***********************************************************************[solvetypes.hpp]
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

#ifndef __SOLVER_TYPES_
#define __SOLVER_TYPES_

#include "memory.hpp"
#include "vector.hpp"
#include "clause.hpp"
#include "space.hpp"
#include "watch.hpp"
#include "heap.hpp"

namespace SeqFROST {
	
    typedef Vec<WATCH>          WL;
    typedef Vec<WL>             WT;
    typedef Vec<uint32>         BOL;
    typedef Vec<C_REF>          WOL;

    struct SCORS_CMP {
        const Vec<WOL>& wot;
        SCORS_CMP(const Vec<WOL>& wot) : wot(wot) { }
        inline bool operator () (const uint32& a, const uint32& b) const;
    };

    struct CSIZE {
        C_REF ref;
        uint32 size;
        inline CSIZE() {}
        inline CSIZE(const C_REF& ref, const uint32& size) : ref(ref), size(size) { }
    };

    struct DFS {
        uint32 idx, min;
        inline DFS() : idx(0), min(0) { }
    };

    typedef uint32 cbucket_t;
    typedef SMM<cbucket_t, C_REF> CTYPE;

    class CMM : public CTYPE
    {
        Vec<bool, C_REF> _stencil;

        #define CLAUSEPTR(REF) (CLAUSE*)CTYPE::address(REF)

    public:
        CMM() { 
            assert(CTYPE::bucket() == 4);
            assert(SOLVER_LITSIZE == sizeof(cbucket_t));
            assert(SOLVER_CLAUSESIZE == sizeof(CLAUSE)); 
            assert(SOLVER_CLAUSEBUCKETS == (SOLVER_CLAUSESIZE / SOLVER_LITSIZE) - 2); 
        }
        explicit				CMM             (const C_REF& init_cap) : CTYPE(init_cap), _stencil(init_cap, 0) { assert(CTYPE::bucket() == 4); }
        inline void				init            (const C_REF& nCls, const C_REF& nLits) { 
            CTYPE::init(INITNBUCKETS(nCls, nLits));
            _stencil.resize(nCls * 2, 0);
        }
        
        inline		 CLAUSE&    operator[]		(const C_REF& r)       { return (CLAUSE&)CTYPE::operator[](r); }
        inline const CLAUSE&    operator[]		(const C_REF& r) const { return (CLAUSE&)CTYPE::operator[](r); }
        inline		 CLAUSE*    clause          (const C_REF& r)       { return CLAUSEPTR(r); }
        inline const CLAUSE*    clause          (const C_REF& r) const { return CLAUSEPTR(r); }
        inline       bool*      stencil         ()                     { assert(_stencil.size()); return _stencil.data(); }
        inline       bool		deleted         (const C_REF& r) const { assert(check(r)); return _stencil[r]; }
        inline       void		collectLiterals (const int& size) { junk += size; }
        inline       void		collectClause   (const C_REF& r, const int& size) { 
            junk += CBUCKETS(size);
            assert(check(r));
            _stencil[r] = true; 
        }
        
        inline       void		migrateTo       (CMM& dest) {
            CTYPE::migrateTo(dest);
            _stencil.migrateTo(dest._stencil);
        }
        template <class SRC>
        inline       CLAUSE*	alloc           (C_REF& r, const SRC& src) {
            assert(src.size() > 1);
            r = CTYPE::alloc(CBUCKETS(src.size()));
            CLAUSE* c = new (CLAUSEPTR(r)) CLAUSE(src);
            assert(c->capacity() == CBUCKETS(src.size()));
            assert(src.size() == clause(r)->size());
            _stencil.expand(r + 1, 0);
            return c;
        }
        inline       CLAUSE*	alloc           (C_REF& r, const int& size) {
            assert(size > 1);
            r = CTYPE::alloc(CBUCKETS(size));
            CLAUSE* c = new (CLAUSEPTR(r)) CLAUSE(size);
            assert(c->capacity() == CBUCKETS(size));
            assert(size == c->size());
            _stencil.expand(r + 1, 0);
            return c;
        }
        inline       void		destroy         () { dealloc(), _stencil.clear(true); }
    };

    typedef Vec<C_REF>          BCNF;
    typedef Vec<CSIZE, C_REF>   csched_t;
    typedef HEAP<SCORS_CMP>     vsched_t;

    #define forall_bol(BLIST, PTR) \
		for (uint32* PTR = BLIST, *END = BLIST.end(); PTR != END; ++PTR)

    #define forall_wol(WLIST, PTR) \
		for (C_REF* PTR = WLIST, *END = WLIST.end(); PTR != END; ++PTR)

    #define PREFETCH_CS(CS) \
        const cbucket_t* CS = cm.address(0); \

    #define PREFETCH_CM(CS,DELETED) \
        const cbucket_t* CS = cm.address(0); \
        const bool* DELETED = cm.stencil(); \

}

#endif