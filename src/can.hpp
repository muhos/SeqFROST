/***********************************************************************[can.hpp]
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

#ifndef __CAN_
#define __CAN_

namespace SeqFROST {

	#define canPreSigmify() (opts.preprocess_en)

	#define canRephase() (opts.rephase_en	&& \
						  stats.conflicts > limit.rephase)

	#define canReduce() (opts.reduce_en		&& \
						 LEARNTS			&& \
						 stats.conflicts >= limit.reduce)

	#define	canProbe() (opts.probe_en		&& \
					   (last.probe.reduces <= stats.reduces) && \
					   (limit.probe <= stats.conflicts))

	#define canTernary() (opts.ternary_en && \
						 ((MAXCLAUSES << 1) < (stats.searchticks + opts.ternary_min_eff)))

	#define canForward() (opts.forward_en	&& \
						 (limit.forward <= stats.conflicts))

	#define canMap() (!LEVEL	&& \
					  INACTIVEVARS > (opts.map_perc * inf.maxVar))
	
}

#endif