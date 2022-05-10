/***********************************************************************[key.hpp]
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

#ifndef __SORT_KEY_
#define __SORT_KEY_

#include "vector.hpp"
#include "definitions.hpp"

namespace SeqFROST {

	//============================//
	//  Default Comparators       //
	//============================//
	template <class T>
	struct DEFAULT_RANK {
		inline T operator () (const T& val) { return val; }
	};

	struct PTR_RANK {
		inline size_t operator () (void* ptr) { return (size_t)ptr; }
	};

	template<class T>
	struct LESS {
		inline bool operator () (const T& x, const T& y) const {
			return x < y;
		}
	};

	template<class T>
	struct GREATER {
		inline bool operator () (const T& x, const T& y) const {
			return x > y;
		}
	};

	//===============================//
	//  Custom Boolean Comparators   //
	//===============================//
	struct LCV_CMP {
		const uint32* scores;
		LCV_CMP(const uint32* _scores) : scores(_scores) {}
		inline bool operator () (const uint32& a, const uint32& b) const {
			const uint32 x = scores[a], y = scores[b];
			if (x < y) return true;
			if (x > y) return false;
			return a < b;
		}
	};
	struct MCV_CMP {
		const uint32* scores;
		MCV_CMP(const uint32* _scores) : scores(_scores) {}
		inline bool operator () (const uint32& a, const uint32& b) const {
			const uint32 x = scores[a], y = scores[b];
			if (x > y) return true;
			if (x < y) return false;
			return a > b;
		}
	};
	struct ACTIV_CMP {
		const score_t& act;
		ACTIV_CMP(const score_t& _act) : act(_act) {}
		inline bool operator () (const uint32& a, const uint32& b) const {
			const double xact = act[a], yact = act[b];
			if (xact < yact) return true;
			if (xact > yact) return false;
			return a > b;
		}
	};
	struct QUEUE_CMP {
		const bump_t& bumped;
		QUEUE_CMP(const bump_t& _bumped) : bumped(_bumped) {}
		inline bool operator () (const uint32& a, const uint32& b) const {
			return bumped[a] < bumped[b];
		}
	};
	struct HIST_LCV_CMP {
		const uVec1D& hist;
		HIST_LCV_CMP(const uVec1D& _hist) : hist(_hist) {}
		inline bool operator () (const uint32& a, const uint32& b) const {
			const uint32 xh = hist[a], yh = hist[b];
			if (xh < yh) return true;
			if (xh > yh) return false;
			return a < b;
		}
	};
	struct HIST_MCV_CMP {
		const uVec1D& hist;
		HIST_MCV_CMP(const uVec1D& _hist) : hist(_hist) {}
		inline bool operator () (const uint32& a, const uint32& b) const {
			const uint32 xh = hist[a], yh = hist[b];
			if (xh > yh) return true;
			if (xh < yh) return false;
			return a < b;
		}
	};
	struct KEY_CMP_ACTIVITY {
		const score_t& acts;
		const uint32* scores;
		KEY_CMP_ACTIVITY(score_t& _acts, const uint32* _scores) :
			acts(_acts), scores(_scores) {}
		inline bool operator()(const uint32& a, const uint32& b) const {
			const double dx = acts[a], dy = acts[b];
			if (dx > dy) return true;
			if (dx < dy) return false;
			const uint32 x = scores[a], y = scores[b];
			if (x > y) return true;
			if (x < y) return false;
			return a > b;
		}
	};
	struct KEY_CMP_BUMP {
		const bump_t& bumped;
		KEY_CMP_BUMP(const bump_t& _bumped) : bumped(_bumped) {}
		inline bool operator()(const uint32& x, const uint32& y) const {
			return bumped[x] > bumped[y];
		}
	};
	//=================================//
	//  Custom Stable Sort Comparators //
	//=================================//
	struct STABLE_LCV {
		const uint32* scores;
		STABLE_LCV(const uint32* _scores) : scores(_scores) {}
		inline bool operator () (const uint32* a, const uint32* b) const {
			return scores[*a] <= scores[*b];
		}
	};
	struct STABLE_MCV {
		const uint32* scores;
		STABLE_MCV(const uint32* _scores) : scores(_scores) {}
		inline bool operator () (const uint32* a, const uint32* b) const {
			return scores[*a] >= scores[*b];
		}
	};
	struct STABLE_QUEUE {
		const bump_t& bumped;
		STABLE_QUEUE(const bump_t& _bumped) : bumped(_bumped) {}
		inline bool operator () (const uint32* a, const uint32* b) const {
			return bumped[*a] <= bumped[*b];
		}
	};
	struct STABLE_ACTIVITY {
		const score_t& acts;
		const uint32* scores;
		STABLE_ACTIVITY(score_t& _acts, const uint32* _scores) :
			acts(_acts), scores(_scores) {}
		inline bool operator()(const uint32* a, const uint32* b) const {
			const uint32 sa = *a, sb = *b;
			const double dx = acts[sa], dy = acts[sb];
			if (dx > dy) return true;
			if (dx < dy) return false;
			return scores[sa] >= scores[sb];
		}
	};
	struct STABLE_BUMP {
		const bump_t& bumped;
		STABLE_BUMP(const bump_t& _bumped) : bumped(_bumped) {}
		inline bool operator()(const uint32* a, const uint32* b) const {
			return bumped[*a] >= bumped[*b];
		}
	};
	//============================//
	//  Custom Rankers	          //
	//============================//
	struct QUEUE_RANK {
		const bump_t& bumped;
		QUEUE_RANK(const bump_t& _bumped) : bumped(_bumped) {}
		inline uint64 operator () (const uint32& a) const {
			return bumped[a];
		}
	};
	struct KEY_RANK_BUMP {
		const bump_t& bumped;
		KEY_RANK_BUMP(const bump_t& _bumped) : bumped(_bumped) {}
		inline uint64 operator()(const uint32& x) const {
			uint64 b = bumped[x];
			return ~b;
		}
	};
	struct LCV_RANK {
		const uint32* scores;
		LCV_RANK(const uint32* _scores) : scores(_scores) {}
		inline uint32 operator () (const uint32& a) const {
			return scores[a];
		}
	};
	struct MCV_RANK {
		const uint32* scores;
		MCV_RANK(const uint32* _scores) : scores(_scores) {}
		inline uint32 operator () (const uint32& a) const {
			return ~scores[a];
		}
	};

}

#endif 