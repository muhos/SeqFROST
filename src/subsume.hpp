/***********************************************************************[subsume.hpp]
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

#ifndef __SUB_
#define __SUB_

#include "simplify.hpp" 

using namespace SeqFROST;

inline void updateOL(SCNF& scnf, OL& ol)
{
	if (ol.empty()) return;
	S_REF* j = ol;
	forall_occurs(ol, i) {
		SCLAUSE& c = scnf[*i];
		if (c.molten()) c.freeze();
		else if (!c.deleted()) *j++ = *i;
	}
	ol.resize(int(j - ol));
}

inline bool sub(SCLAUSE& subsuming, SCLAUSE& subsumed)
{
	assert(!subsuming.deleted());
	assert(!subsumed.deleted());
	assert(subsuming.size() > 1);
	assert(subsumed.size() > 1);
	assert(subsuming.size() <= subsumed.size());
	const int size = subsuming.size();
	uint32* d1 = subsuming.data(), *d2 = subsumed.data();
	uint32* e1 = d1 + size;
	uint32* e2 = subsumed.end();
	int sub = 0;
	while (d1 != e1 && d2 != e2) {
		const uint32 lit1 = *d1, lit2 = *d2;
		if (lit1 < lit2) 
			d1++;
		else if (lit2 < lit1) 
			d2++;
		else { 
			sub++; 
			d1++, d2++;
		}
	}
	if (sub == size) 
		return true;
	return false;
}

inline bool sub(Lits_t& subsuming, SCLAUSE& subsumed)
{
	assert(!subsumed.deleted());
	assert(subsuming.size() > 1);
	assert(subsumed.size() > 1);
	assert(subsuming.size() <= subsumed.size());
	const int size = subsuming.size();
	uint32* d1 = subsuming.data(), *d2 = subsumed.data();
	uint32* e1 = d1 + size;
	uint32* e2 = subsumed.end();
	int sub = 0;
	while (d1 != e1 && d2 != e2) {
		const uint32 lit1 = *d1, lit2 = *d2;
		if (lit1 < lit2) 
			d1++;
		else if (lit2 < lit1) 
			d2++;
		else { 
			sub++; 
			d1++, d2++;
		}
	}
	if (sub == size) 
		return true;
	return false;
}

inline bool selfsub(const uint32& x, const uint32& fx, SCLAUSE& subsuming, SCLAUSE& subsumed)
{
	assert(!subsuming.deleted());
	assert(!subsumed.deleted());
	assert(subsuming.size() > 1);
	assert(subsumed.size() > 1);
	assert(subsuming.size() <= subsumed.size());
	const int size = subsuming.size();
	uint32* d1 = subsuming.data();
	uint32* d2 = subsumed.data();
	uint32* e1 = d1 + size;
	uint32* e2 = subsumed.end();
	int sub = 0;
	bool self = false;
	while (d1 != e1 && d2 != e2) {
		const uint32 lit1 = *d1;
		const uint32 lit2 = *d2;
		if (lit1 == fx) 
			d1++;
		else if (lit2 == x) { 
			self = true; 
			d2++; 
		}
		else if (lit1 < lit2)
			d1++;
		else if (lit2 < lit1)
			d2++;
		else { 
			sub++; 
			d1++, d2++; 
		}
	}
	if ((sub + 1) == size) {
		if (self) 
			return true;
		else {
			while (d2 != e2) {
				if (*d2 == x) 
					return true;
				d2++;
			}
		}
	}
	return false;
}

inline bool subsume(SCNF& scnf, OL& list, S_REF* end, SCLAUSE& cand)
{
	const int candsz = cand.size();
	for (S_REF* j = list; j != end; ++j) {
		SCLAUSE& subsuming = scnf[*j];
		if (subsuming.deleted()) continue;
		if (cand.molten() && subsuming.size() > candsz) continue;
		if (subsuming.size() > 1 && sub(subsuming.sig(), cand.sig()) && sub(subsuming, cand)) {
			if (subsuming.learnt() && cand.original()) 
				subsuming.set_status(ORIGINAL);
			solver->removeClause(cand);
			LOGCLAUSE(4, cand, " Clause ");
			LOGCLAUSE(4, subsuming, " Subsumed by ");
			return true;
		}
	}
	return false;
}

inline bool selfsubsume(const uint32& x, const uint32& fx, SCNF& scnf, OL& list, SCLAUSE& cand)
{
	// try to strengthen 'cand' by removing 'x'
	const int candsz = cand.size();
	const uint32 candsig = cand.sig();
	forall_occurs(list, j) {
		SCLAUSE& subsuming = scnf[*j];
		const int subsize = subsuming.size();
		if (subsize > candsz) break;
		if (subsuming.deleted()) continue;
		if (subsize > 1 && selfsub(subsuming.sig(), candsig) && selfsub(x, fx, subsuming, cand)) {
			LOGCLAUSE(4, cand, " Clause ");
			solver->strengthen(cand, x);
			cand.melt(); // mark for fast recongnition in ot update 
			LOGCLAUSE(4, subsuming, " Strengthened by ");
			return true; // cannot strengthen "cand" anymore, 'x' already removed
		}
	}
	return false;
}

inline void self_sub_x(const uint32& p, const int& maxClauseSize, SCNF& scnf, OL& poss, OL& negs, SUBSTATS& substats)
{
	CHECKLIT(p);
	assert(checkMolten(scnf, poss, negs));
	const uint32 n = NEG(p);

	// positives vs negatives
	forall_occurs(poss, i) {
		SCLAUSE& pos = scnf[*i];
		if (pos.size() > maxClauseSize) break;
		if (pos.deleted()) continue;
#ifdef STATISTICS
		if (selfsubsume(p, n, scnf, negs, pos)) substats.strengthened++;
		if (subsume(scnf, poss, i, pos)) substats.subsumed++;
#else 
		selfsubsume(p, n, scnf, negs, pos);
		subsume(scnf, poss, i, pos);
#endif
	}

	updateOL(scnf, poss);

	// negatives vs positives
	forall_occurs(negs, i) {
		SCLAUSE& neg = scnf[*i];
		if (neg.size() > maxClauseSize) break;
		if (neg.deleted()) continue;
#ifdef STATISTICS
		if (selfsubsume(n, p, scnf, poss, neg)) substats.strengthened++;
		if (subsume(scnf, negs, i, neg)) substats.subsumed++;
#else
		selfsubsume(n, p, scnf, poss, neg);
		subsume(scnf, negs, i, neg);
#endif
	}

	updateOL(scnf, negs);

	assert(checkMolten(scnf, poss, negs));
	assert(checkDeleted(scnf, poss, negs));
}

#endif