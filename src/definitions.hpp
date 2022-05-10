/***********************************************************************[definitions.hpp]
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

#ifndef __GLOBAL_DEFS_
#define __GLOBAL_DEFS_

//=======================================//
//            C++ directives             //
//=======================================//
#include <iostream>
#include <fstream>
#include <cstring>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <csignal>
#include <cstdio>
#include <cmath>
#include "check.hpp"
#include "logging.hpp"
#include "datatypes.hpp"
#include "constants.hpp"

#if defined(__linux__)
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <cpuid.h>
#elif defined(__CYGWIN__)
#include </usr/include/sys/resource.h>
#include </usr/include/sys/mman.h>
#include </usr/include/sys/sysinfo.h>
#include </usr/include/sys/unistd.h>
#elif defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#include <intrin.h>
#include <Winnt.h>
#include <io.h>
#endif
#undef ERROR
#undef hyper 
#undef SET_BOUNDS

using std::string;
using std::ifstream;

namespace SeqFROST {

	//=========================================//
	//              Global CNF INFO            //
	//=========================================//

	struct CNF_INFO {
		uint32 orgVars, maxVar, maxFrozen, maxMelted, maxSubstituted, nDualVars, unassigned;
		uint32 orgCls, nDeletedVarsAfter, nClausesAfter, nLiteralsAfter, nClauses, nLiterals;
		CNF_INFO() { RESETSTRUCT(this); }
	};
	extern CNF_INFO inf;

	//=========================================//
	//                 MACROS                  //
	//=========================================//

	#define CHECKVAR(VAR) assert(_checkvar(VAR, inf.maxVar))

	#define CHECKLIT(LIT) assert(_checklit(LIT, inf.nDualVars))

	#define CHECKLEVEL(LVL) assert(_checklvl(LVL, UNDEF_LEVEL))

	#define INACTIVEVARS (inf.maxMelted + inf.maxFrozen + inf.maxSubstituted)

	#define ACTIVEVARS (inf.maxVar - INACTIVEVARS)

	#define forall_variables(VAR) for (uint32 VAR = 1; VAR <= inf.maxVar; ++VAR)

	#define forall_literals(LIT) for (uint32 LIT = 2; LIT < inf.nDualVars; ++LIT)

	#define forall_vector(TYPE, VEC, PTR) \
		for (TYPE* PTR = VEC, *VEND = VEC.end(); PTR != VEND; ++PTR)

	#define forall_clause(C, PTR) \
		for (uint32* PTR = C, *CEND = C.end(); PTR != CEND; ++PTR)

	#define forall_cnf(INCNF, PTR) \
		for (C_REF* PTR = INCNF, *CNFEND = INCNF.end(); PTR != CNFEND; ++PTR)
	
	//====================================================//
	//                 Global Inline helpers              //
	//====================================================//
	template<class T>
	inline bool		eq				(T& in, arg_t ref) {
		while (*ref) { if (*ref != *in) return false; ref++; in++; }
		return true;
	}
	template<class T>
	inline bool		eqn				(T in, arg_t ref, const bool& lower = false) {
		if (lower) {
			while (*ref) { 
				if (tolower(*ref) != tolower(*in))
					return false; 
				ref++; in++;
			}
		}
		else {
			while (*ref) { if (*ref != *in) return false; ref++; in++; }
		}
		return true;
	}
	inline bool		hasstr			(const char* in, const char* ref)
	{
		size_t count = 0;
		const size_t reflen = strlen(ref);
		while (*in) {
			if (ref[count] != *in)
				count = 0;
			else
				count++;
			in++;
			if (count == reflen)
				return true;
		}
		return false;
	}
	inline double	ratio			(const double& x, const double& y) { return y ? x / y : 0; }
	inline uint64	ratio			(const uint64& x, const uint64& y) { return y ? x / y : 0; }
	inline double	percent			(const double& x, const double& y) { return ratio(100 * x, y); }
	inline int		l2i				(const uint32& lit) { return SIGN(lit) ? -int(ABS(lit)) : int(ABS(lit)); }
	
}

#endif 

