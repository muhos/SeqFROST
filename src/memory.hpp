/***********************************************************************[memory.hpp]
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

#ifndef __MEMORY_
#define __MEMORY_

#include "logging.hpp"
#include "malloc.hpp"
#include <cstdint>
#include <limits>
#include <cassert>

namespace SeqFROST {
    /******************************************************/
    /*  Usage: global memory manager with garbage monitor */
    /*         and boundary checker                       */
    /*  Dependency:  none                                 */
    /******************************************************/
    template<class T, class S = size_t>
    class SMM
    {
        T* _mem;
        S sz, cap;
        const S maxCap;

    protected:

        S junk;

        bool check(const S& d) const {
            if (d >= sz) {
                LOGERRN("memory index (%lld) violates memory boundary (%lld)", uint64(d), uint64(sz));
                return false;
            }
            return true;
        }
        bool check(const G_REF& d) const {
            if (d < _mem) {
                LOGERRN("memory access violation at location: %p, base address: %p", d, _mem);
                return false;
            }
            if (d > (sz - 1) + _mem) {
                LOGERRN("memory access violation at location: %p, end address: %p", d, ((sz - 1) + _mem));
                return false;
            }
            return true;
        }
        bool checkSize(const S& newSz) const {
            if (sz != 0 && newSz <= sz) {
                LOGERRN("size overflow during memory allocation: (new = %lld, old = %lld)", uint64(newSz), uint64(sz));
                return false;
            }
            return true;
        }
    public:
                        ~SMM        () { dealloc(); }
                        SMM         () :
                            _mem(NULL)
                            , sz(0)
                            , cap(0)
                            , maxCap(std::numeric_limits<S>::max())
                            , junk(0)
        { }
        explicit        SMM         (const S& _cap) :
                            _mem(NULL)
                            , sz(0)
                            , cap(0)
                            , maxCap(std::numeric_limits<S>::max())
                            , junk(0)
        {
            assert(maxCap > INT8_MAX);
            init(_cap);
        }
        inline size_t   bucket      () const { return sizeof(T); }
        inline S        size        () const { return sz; }
        inline S        capacity    () const { return cap; }
        inline S        garbage     () const { return junk; }
        inline T&       operator[]  (const S& idx) { assert(check(idx)); return _mem[idx]; }
        inline const T& operator[]  (const S& idx) const { assert(check(idx)); return _mem[idx]; }
        inline T*       address     (const S& idx) { assert(check(idx)); return _mem + idx; }
        inline const T* address     (const S& idx) const { assert(check(idx)); return _mem + idx; }
        inline void     init        (const S& init_cap) {
            if (!init_cap) return;   
            if (init_cap > maxCap) {
                LOGERRN("initial size exceeds maximum memory size: (max = %lld, size = %lld)", uint64(maxCap), uint64(init_cap));
                throw MEMOUTEXCEPTION();
            }
            cap = init_cap;
            sfralloc(_mem, sizeof(T) * cap);
        }
        inline S        alloc       (const S& size) {
            assert(size > 0);
            const S newSz = sz + size;
            assert(checkSize(newSz));
            RESERVE(T, maxCap, _mem, cap, newSz);
            S oldSz = sz;
            sz += size;
            assert(sz > 0);
            return oldSz;
        }
        inline void     migrateTo   (SMM& newBlock) {
            if (newBlock._mem) 
                std::free(newBlock._mem);
            newBlock._mem = _mem, newBlock.sz = sz, newBlock.cap = cap, newBlock.junk = junk;
            _mem = NULL, sz = 0, cap = 0, junk = 0;
        }
        inline void     dealloc     () {
            if (_mem) 
                std::free(_mem), _mem = NULL;
            sz = cap = 0;
            junk = 0;
        }
    };

}

#endif