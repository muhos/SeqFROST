/***********************************************************************[constants.hpp]
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

#ifndef __CONSTANTS_
#define __CONSTANTS_

#include "datatypes.hpp"
#include "cstring"

extern bool quiet_en;
extern bool competition_en;
extern int  verbose;

namespace SeqFROST {

	//=======================================//
	//             Solver Macros           //
	//=======================================//
	constexpr int INIT_CAP			= 128;
	constexpr int UNDEFINED			= -1;
	constexpr int AWAKEN_SUCC		= 0;
	constexpr int AWAKEN_FAIL		= 1;
	constexpr uint32 MBYTE			= 0x00100000;
	constexpr uint32 KBYTE			= 0x00000400;
	constexpr uint32 GBYTE			= 0x40000000;
	constexpr uint32 MAX_DLC		= 0x00000002;
	constexpr uint32 MAX_LBD		= 0x04000000;
	constexpr uint32 MAX_LBD_M		= 0x03FFFFFF;
	constexpr uint32 MAX_MARKER		= 0xFFFFF000;
	constexpr uint32 NEG_SIGN		= 0x00000001;
	constexpr uint32 HASH_MASK		= 0x0000001F;
	constexpr uint32 UNDEF_VAR		= 0xFFFFFFFF;
	constexpr uint32 UNDEF_LEVEL	= 0xFFFFFFFF;
	constexpr C_REF  UNDEF_REF		= 0xFFFFFFFFFFFFFFFF;
	constexpr LIT_ST UNDEF_VAL		= -1;
	constexpr LIT_ST ORGPHASE		= 1;
	constexpr LIT_ST INVPHASE		= 2;
	constexpr LIT_ST FLIPPHASE		= 3;
	constexpr LIT_ST BESTPHASE		= 4;
	constexpr LIT_ST RANDPHASE		= 5;
	constexpr LIT_ST WALKPHASE		= 6;
	constexpr LIT_ST NOVAL_MASK		= -2;
	constexpr LIT_ST VAL_MASK		= 1;
	constexpr LIT_ST MELTED_M		= 0x01;
	constexpr LIT_ST FROZEN_M		= 0x02;
	constexpr LIT_ST SUBSTITUTED_M	= 0x04;
	constexpr LIT_ST ANALYZED_M		= 0x01;
	constexpr LIT_ST REMOVABLE_M	= 0x02;
	constexpr LIT_ST POISONED_M		= 0x04;
	constexpr LIT_ST SHRINKABLE_M	= 0x08;
	constexpr CL_ST USAGET3			= 0x01;
	constexpr CL_ST USAGET2			= 0x02;
	constexpr CL_ST USAGET1			= 0x03;
	constexpr CL_ST ORIGINAL		= 0x00;
	constexpr CL_ST LEARNT			= 0x01;
	constexpr CL_ST DELETED			= 0x02;
	constexpr CNF_ST UNSAT_M		= 0;
	constexpr CNF_ST SAT_M			= 1;
	constexpr CNF_ST UNSOLVED_M		= 2;

	#define POS(x)			((x) & 0xFFFFFFFE)
	#define ABS(x)			((x) >> 1)
	#define V2L(x)			((x) << 1)
	#define SIGN(x)			LIT_ST((x) & NEG_SIGN)
	#define NEG(x)			((x) | NEG_SIGN)
	#define V2DEC(x,s)		(V2L(x) | (s))
	#define FLIP(x)			((x) ^ NEG_SIGN)
	#define HASH(x)			((x) & HASH_MASK)
	#define MAPHASH(x)		(1UL << HASH(x))
	#define MELTED(x)		((x) & MELTED_M)
	#define FROZEN(x)		((x) & FROZEN_M)
	#define SUBSTITUTED(x)	((x) & SUBSTITUTED_M)
	#define ANALYZED(x)		((x) & ANALYZED_M)
	#define	REMOVABLE(x)	((x) & REMOVABLE_M)	
	#define	POISONED(x)		((x) & POISONED_M)
	#define	SHRINKABLE(x)	((x) & SHRINKABLE_M)
	#define UNASSIGNED(x)	((x) & NOVAL_MASK)
	#define REASON(x)		((x) ^ UNDEF_REF)
	#define DECISION(x)		(!REASON(x))
	#define NEQUAL(x,y)		((x) ^ (y))
	#define CACHELINES(x)	((x) >> 2)
	#define MIN(x,y)		((x) < (y) ? (x) : (y))
	#define MAX(x,y)		((x) > (y) ? (x) : (y))
	#define RESETSTRUCT(MEMPTR) \
		memset(MEMPTR, 0, sizeof(*MEMPTR));

}

#endif