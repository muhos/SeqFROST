/***********************************************************************[dimacs.hpp]
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

#ifndef __DIMACS_
#define __DIMACS_

#include "definitions.hpp"
#include "timer.hpp"
#include <climits>
#include <fcntl.h>
#include <sys/stat.h>

namespace SeqFROST {

	struct FORMULA {
		string path;
		double c2v;
		uint64 size;
		uint32 units, large, binaries, ternaries;
		int maxClauseSize;
		FORMULA() : 
			path()
			, c2v(0)
			, size(0)
			, units(0)
			, large(0)
			, binaries(0)
			, ternaries(0)
			, maxClauseSize(0) {}
		FORMULA(const string& path) :
			path(path)
			, c2v(0)
			, size(0)
			, units(0)
			, large(0)
			, binaries(0)
			, ternaries(0)
			, maxClauseSize(0) {}
	};

	#define isDigit(CH) (((CH) ^ '0') <= 9)

	#define eatWS(STR) { while ((*STR >= 9 && *STR <= 13) || *STR == 32) STR++; }

	#define eatLine(STR) { while (*STR && *STR++ != '\n'); }

	inline uint32 toInteger(char*& str, uint32& sign)
	{
		eatWS(str);
		sign = 0;
		if (*str == '-') sign = 1, str++;
		else if (*str == '+') str++;
		if (!isDigit(*str)) LOGERR("expected a digit but ASCII(%d) is found", *str);
		uint32 n = 0;
		while (isDigit(*str)) n = n * 10 + (*str++ - '0');
		return n;
	}

	inline bool canAccess(const char* path, struct stat& st)
	{
		if (stat(path, &st)) return false;
#ifdef _WIN32
#define R_OK 4
		if (_access(path, R_OK)) return false;
#else
		if (access(path, R_OK)) return false;
#endif
		return true;
	}

}

#endif 