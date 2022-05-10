/***********************************************************************[proof.cpp]
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

#include "proof.hpp"
#include "sort.hpp"

using namespace SeqFROST;


constexpr size_t BUFFERSIZE = 1.5 * MBYTE;
constexpr size_t MARGINSIZE = 1 * MBYTE;

#define BYTEMAX			 128
#define IBYTEMAX		-128
#define BYTEMASK		 127
#define L2B(X)			(((X) & BYTEMASK) | BYTEMAX)
#define ISLARGE(X)		((X) & IBYTEMAX)
#define writebyte(B)	buffer.push(B)
#define	write(LITS,LEN)						\
{											\
	assert(checkFile());					\
	assert(LEN > 0);						\
	if (nonbinary_en) nonbinary(LITS, LEN); \
	else binary(LITS, LEN);					\
	added++;								\
}
#define addline(LITS,LEN)					\
{											\
	assert(checkFile());					\
	if (!nonbinary_en) writebyte('a');		\
	write(LITS, LEN);						\
}
#define delline(LITS,LEN)					\
{											\
	assert(checkFile());					\
	writebyte('d');							\
	if (nonbinary_en) writebyte(' ');		\
	write(LITS, LEN);						\
}

PROOF::PROOF() : 
	proofFile(NULL)
	, sp(NULL)
	, vars(NULL)
	, added(0)
	, nonbinary_en(false)
{
	buffer.reserve(BUFFERSIZE);
}

PROOF::~PROOF()
{ 
	assert(buffer.empty());
	buffer.clear(true);
	clause.clear(true);
	tmpclause.clear(true);
	vars = NULL, sp = NULL, proofFile = NULL;
}

void PROOF::close()
{
	if (buffer.size()) {
		fwrite(buffer.data(), 1, buffer.size(), proofFile);
		buffer.clear();
	}
	if (proofFile) {
		fclose(proofFile);
		proofFile = NULL;
	}
}

void PROOF::handFile(arg_t path, const bool& _nonbinary_en)
{
	LOGN2(1, " Handing over \"%s%s%s\" to the proof system..", CREPORTVAL, path, CNORMAL);
	proofFile = fopen(path, "w");
	if (proofFile == NULL) LOGERR("cannot open proof file %s", path);
	nonbinary_en = _nonbinary_en;
	LOGENDING(1, 5, "(binary %s)", nonbinary_en ? "disabled" : "enabled");
}

void PROOF::init(SP* _sp)
{
	assert(_sp);
	sp = _sp;
}

void PROOF::init(SP* _sp, uint32* vorg)
{
	assert(_sp);
	assert(vorg);
	sp = _sp;
	vars = vorg;
}

inline bool PROOF::checkFile()
{
	if (proofFile == NULL) {
		LOGERRN("proof file is not opened or cannot be accessed");
		return false;
	}
	return true;
}

inline void PROOF::binary(const uint32* lits, const int& len)
{
	const uint32 buffsize = buffer.size();
	if (buffsize && buffsize > MARGINSIZE) {
		fwrite(buffer.data(), 1, buffsize, proofFile);
		buffer.clear();
	}
	for (int i = 0; i < len; ++i) {
		const uint32 lit = lits[i];
		CHECKLIT(lit);
		uint32 r = V2DEC(vars[ABS(lit)], SIGN(lit));
		assert(r > 1 && r < UNDEF_VAR);
		Byte b;
		while (ISLARGE(r)) {
			b = L2B(r);
			writebyte(b);
			r >>= 7;
		}
		b = r;
		writebyte(b);
	}
	writebyte(0);
}

inline void PROOF::nonbinary(const uint32* lits, const int& len)
{
	const size_t bytes = 16;
	const char delimiter = '0';
	char line[bytes];
	char* tail = line + bytes;
	*--tail = 0;
	for (int i = 0; i < len; ++i) {
		const uint32 lit = lits[i];
		CHECKLIT(lit);
		const uint32 mvar = vars[ABS(lit)];
		const LIT_ST sign = SIGN(lit);
		if (sign) writebyte('-');
		char* nstr = tail;
		assert(!*nstr);
		uint32 digit = mvar;
		while (digit) {
			*--nstr = (digit % 10) + delimiter;
			digit /= 10;
		}
		while (nstr != tail) writebyte(*nstr++);
		writebyte(' ');
	}
	writebyte(delimiter);
	writebyte('\n');
}

void PROOF::addEmpty() 
{ 
	assert(checkFile());
	if (nonbinary_en) {
		writebyte('0');
	}
	else {
		writebyte('a');
		writebyte(0);
	}
	added++;
}

inline void PROOF::addClause() { addline(clause.data(), clause.size()); clause.clear(); }

inline void PROOF::deleteClause() { delline(clause.data(), clause.size()); clause.clear(); }

void PROOF::addUnit(uint32 unit) { addline(&unit, 1); }

void PROOF::addClause(Lits_t& c) { addline(c.data(), c.size()); }

void PROOF::addClause(CLAUSE& c) { addline(c.data(), c.size()); }

void PROOF::addClause(SCLAUSE& c) { addline(c.data(), c.size()); }

void PROOF::deleteClause(Lits_t& c) { delline(c.data(), c.size()); }

void PROOF::deleteClause(CLAUSE& c) { delline(c.data(), c.size()); }

void PROOF::deleteClause(SCLAUSE& c) { delline(c.data(), c.size()); }

void PROOF::deleteClause(SCLAUSE& c, const uint32& def, const uint32& other)
{
	assert(clause.empty());
	forall_clause(c, k) {
		const uint32 lit = *k;
		CHECKLIT(lit);
		if (NEQUAL(lit, def)) clause.push(lit);
		else clause.push(other);
	}
	assert(clause.size() == c.size());
	deleteClause();
}

void PROOF::addResolvent(SCLAUSE& c)
{
	assert(clause.empty());
	assert(sp != NULL);
	const LIT_ST* values = sp->value;
	forall_clause(c, k) {
		const uint32 lit = *k;
		CHECKLIT(lit);
		if (values[lit] > 0) {
			clause.clear();
			return;
		}
		clause.push(lit);
	}
	addClause();
}

void PROOF::shrinkClause(CLAUSE& c, const uint32& me)
{
	assert(clause.empty());
	forall_clause(c, k) {
		const uint32 lit = *k;
		CHECKLIT(lit);
		if (NEQUAL(lit, me)) clause.push(lit);
	}
	assert(clause.size() == c.size() - 1);
	addClause();
	deleteClause(c);
}

void PROOF::shrinkClause(SCLAUSE& c, const uint32& me)
{
	assert(clause.empty());
	forall_clause(c, k) {
		const uint32 lit = *k;
		CHECKLIT(lit);
		if (NEQUAL(lit, me)) clause.push(lit);
	}
	assert(clause.size() == c.size() - 1);
	addClause();
	deleteClause(c);
}

void PROOF::shrinkClause(CLAUSE& c)
{
	assert(clause.empty());
	assert(sp != NULL);
	const uint32* levels = sp->level;
	forall_clause(c, k) {
		const uint32 lit = *k;
		const uint32 v = ABS(lit);
		CHECKLIT(lit);
		if (!levels[v]) {
			assert(!sp->value[lit]);
			continue;
		}
		clause.push(lit);
	}
	assert(clause.size() > 1);
	addClause();
	deleteClause(c);
}

void PROOF::checkInput(arg_t input)
{
	LOGN2(1, " Handing test clause \"%s%s%s\" to the proof system..", CREPORTVAL, input, CNORMAL);
	while (*input) {
		while (isspace(*input)) input++;
		uint32 v = 0, s = 0;
		if (*input == '-') s = 1, input++;
		else if (*input == '+') input++;
		while (isdigit(*input)) v = v * 10 + (*input++ - '0');
		if (v) {
			uint32 lit = V2DEC(v, s);
			CHECKLIT(lit);
			tmpclause.push(lit);
		}
	}
	SORT(tmpclause);
	LOGDONE(1, 5);
}

void PROOF::printClause(const char* msg, const uint32* lits, const int& len, const bool& map)
{
	LOGN1(" Proof: %s clause(", msg);
	for (int i = 0; i < len; ++i) {
		int lit = int(ABS(lits[i]));
		PRINT("%4d ", SIGN(lits[i]) ? -lit : lit);
	}
	PRINT(")\n");
	if (map) {
		LOGN1(" Proof: %s mapped clause(", msg);
		for (int i = 0; i < len; ++i) {
			const uint32 lit = lits[i];
			const uint32 mvar = vars[ABS(lit)];
			PRINT("%4d ", SIGN(lit) ? -int(mvar) : int(mvar));
		}
		PRINT(")\n");
	}
}