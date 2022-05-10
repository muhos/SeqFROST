/***********************************************************************[datatypes.hpp]
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

#ifndef __DATATYPES_
#define __DATATYPES_

namespace SeqFROST {

	// primitive types
	typedef		const char*			arg_t;
	typedef		unsigned char		Byte;
	typedef		Byte*				addr_t;
	typedef		signed char			CL_ST;
	typedef		signed char			CNF_ST;
	typedef		signed char			LIT_ST;
	typedef		unsigned			uint32;
	typedef		signed long long	int64;
	typedef		unsigned long long  uint64;
	typedef		size_t				C_REF;
	typedef		void*				G_REF;

}

#endif