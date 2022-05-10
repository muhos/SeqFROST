/***********************************************************************[level.hpp]
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

#ifndef __LEVEL_
#define __LEVEL_

#include "vector.hpp"

namespace SeqFROST {

	struct level_t {

		uint32 depth;
		uint32 decision;

		level_t() : depth(0), decision(0) { }

		level_t(const uint32& depth, const uint32& decision) :
			depth(depth), decision(decision) { }

	};

	typedef Vec<level_t> dlevel_t;
}

#endif