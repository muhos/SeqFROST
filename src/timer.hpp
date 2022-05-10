/***********************************************************************[timer.hpp]
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

#ifndef __TIMER_
#define __TIMER_

#include <chrono>
#include "constants.hpp"

using namespace std::chrono;

namespace SeqFROST {

#if defined(__linux__) || defined(__CYGWIN__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif

	class TIMER {

		high_resolution_clock::time_point _start, _stop;
		high_resolution_clock::time_point _startp, _stopp;
		double _cpuTime;

	public:
		double parse, solve, simplify;
		double vo, ve, sub, bce, ere, cot, rot, sot, gc, io;
				TIMER		() { RESETSTRUCT(this); }
		void	start		() { _start = high_resolution_clock::now(); }
		void	stop		() { _stop = high_resolution_clock::now();}
		double	cpuTime		() { return _cpuTime = duration_cast<duration<double>>(_stop - _start).count(); }
		void	pstart		() { _startp = high_resolution_clock::now(); }
		void	pstop		() { _stopp = high_resolution_clock::now(); }
		double	pcpuTime	() { return _cpuTime = duration_cast<duration<double>>(_stopp - _startp).count() * 1000.0; }
	};

#if defined(__linux__) || defined(__CYGWIN__)
#pragma GCC diagnostic pop
#endif

}

#endif 

