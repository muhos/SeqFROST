/***********************************************************************[model.cpp]
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

#include "model.hpp"
#include "dimacs.hpp"

using namespace SeqFROST;

void MODEL::printClause(const Lits_t& clause, const bool& printvalues) 
{
	PRINT("%s(", CLOGGING);
	for (int i = 0; i < clause.size(); ++i) {
		PRINT("%-8d", lit2int(clause[i]));
	}
	PRINT(")%s\n", CNORMAL);
	if (printvalues) {
		LOGN1("\t\t\t%s original values(", CLOGGING);
		for (int i = 0; i < clause.size(); ++i) {
			const uint32 orgvar = ABS(clause[i]);
			CHECKVAR(orgvar);
			const uint32 mlit = V2DEC(vorg[orgvar], SIGN(clause[i]));
			PRINT("%-8d", orgvalues[mlit]);
		}
		PRINT(")%s\n", CNORMAL);
		LOGN1("\t\t\t%s extended values(", CLOGGING);
		for (int i = 0; i < clause.size(); ++i) {
			PRINT("%-8d", value[ABS(clause[i])]);
		}
		PRINT(")%s\n", CNORMAL);
	}
}

void MODEL::printValues() 
{
	LOGN2(2, " %sAssigned(size = %d)->(", CLOGGING, maxVar);
	for (uint32 v = 1; v <= maxVar; ++v)
		PRINT("%d:%d  ", v, value[v]);
	PRINT(")%s\n", CNORMAL);
}

void MODEL::printResolved()
{
	LOGN2(2, " %sResolved(size = %d)->(", CLOGGING, resolved.size());
	for (uint32 i = 0; i < resolved.size(); ++i) {
		const int r = resolved[i];
		PRINT("%d  ", r);
	}
	PRINT(")%s\n", CNORMAL);
}

void MODEL::print()
{
	if (!extended) {
		LOGERRN("model is not extended yet");
		return;
	}
	PRINT("v ");
	const LIT_ST* newvalues = value.data();
	for (uint32 v = 1; v <= maxVar; ++v) {
		PRINT("%c%d ", newvalues[v] ? ' ' : '-', v);
		breakmodel(v, 15);
	}
	PUTCH('\n');
	if (!quiet_en) LOG0("");
}

void MODEL::init(uint32* _vorg)
{
	assert(_vorg);
	vorg = _vorg;
	if (!maxVar) {
		assert(inf.maxVar);
		LOG2(2, " Initially mapping original variables to literals..");
		maxVar = inf.maxVar;
		lits.resize(maxVar + 1), lits[0] = 0;
		forall_variables(v) {
			lits[v] = V2L(v);
		}
	}
}

void MODEL::extend(LIT_ST* currValue)
{
	if (extended) return;
	assert(orgvalues == NULL);
	orgvalues = currValue;
    uint32 updated = 0;
    value.resize(maxVar + 1, 0);
	LIT_ST* newvalues = value.data();
    for (uint32 v = 1; v <= maxVar; ++v) {
        const uint32 mlit = lits[v];
        if (mlit && !UNASSIGNED(orgvalues[mlit])) {
			newvalues[v] = orgvalues[mlit];
            updated++;
        }
    }
	LOG2(2, " ");
	LOG2(2, " Extending model updated %d mapped values", updated);
	if (resolved.size()) {
		const uint32 before = updated;
		uint32* begin = resolved, *x = resolved.end() - 1, k;
		while (x > begin) {
			bool unsat = true;
			for (k = *x--; k > 1; --k, --x) {
				if (satisfied(*x)) { unsat = false; break; }
			}
			if (unsat) {
				newvalues[ABS(*x)] = !SIGN(*x);
				updated++;
			}
			x -= k;
		}
		LOG2(2, " Extending model with eliminated variables updated %d values", updated - before);
	}
	extended = true;
	LOG2(2, " ");
}

void MODEL::verify(const string& path) {
	if (!extended) {
		LOGERRN("model is not extended yet");
		return;
	}
	LOG2(1, " ");
	LOG2(1, " Verifying model on input formula..");
	struct stat st;
	if (!canAccess(path.c_str(), st)) LOGERR("cannot access the input file to verify model");
	size_t fsz = st.st_size;
	LOG2(1, "  parsing CNF file \"%s%s%s\" (size: %s%zd KB%s) to verify model",
		CREPORTVAL, path.c_str(), CNORMAL, CREPORTVAL, fsz / KBYTE, CNORMAL);
	TIMER timer;
	timer.start();
#if defined(__linux__) || defined(__CYGWIN__)
	int fd = open(path.c_str(), O_RDONLY, 0);
	if (fd == -1) LOGERR("cannot open input file");
	void* buffer = mmap(NULL, fsz, PROT_READ, MAP_PRIVATE, fd, 0);
	char* str = (char*)buffer;
#else
	ifstream inputFile;
	inputFile.open(path, ifstream::in);
	if (!inputFile.is_open()) LOGERR("cannot open input file to verify model");
	char* buffer = sfcalloc<char>(fsz + 1), * str = buffer;
	inputFile.read(buffer, fsz);
	buffer[fsz] = '\0';
#endif
	char* eof = str + fsz;
	while (str < eof) {
		eatWS(str);
		if (*str == '\0' || *str == '0' || *str == '%') break;
		if (*str == 'c') { eatLine(str); }
		else if (*str == 'p') {
			if (!eq(str, "p cnf")) LOGERR("header has wrong format");
			uint32 sign = 0;
			orgVars = toInteger(str, sign);
			if (sign) LOGERR("number of variables in header is negative");
			if (orgVars == 0) LOGERR("zero number of variables in header");
			if (orgVars >= INT_MAX - 1) LOGERR("number of variables not supported");
			orgClauses = toInteger(str, sign);
			if (sign) LOGERR("number of clauses in header is negative");
			if (orgClauses == 0) LOGERR("zero number of clauses in header");
			LOG2(1, "  found header %s%d %d%s", CREPORTVAL, orgVars, orgClauses, CNORMAL);
			if (orgVars != maxVar) {
				LOGERRN("variables in header inconsistent with model variables");
				verified = false;
				break;
			}
			marks.resize(orgVars + 1, UNDEF_VAL);
		}
		else if (!verify(str)) { verified = false; break; }
	}
#if defined(__linux__) || defined(__CYGWIN__)
	if (munmap(buffer, fsz) != 0) LOGERR("cannot clean input file %s mapping", path.c_str());
	close(fd);
#else
	std::free(buffer);
	inputFile.close();
#endif
	timer.stop();
	timer.parse = timer.cpuTime();
	LOG2(1, "  checked %s%d Variables%s, %s%d Clauses%s, and %s%d Literals%s in %s%.2f seconds%s\nc",
		CREPORTVAL, orgVars, CNORMAL,
		CREPORTVAL, orgClauses, CNORMAL,
		CREPORTVAL, orgLiterals, CNORMAL,
		CREPORTVAL, timer.parse, CNORMAL);
	if (verified) {
		PRINT("c model %sVERIFIED%s\n", CGREEN, CNORMAL);
	}
	else {
		PRINT("c model %sNOT VERIFIED%s\n", CRED, CNORMAL);
	}
}

bool MODEL::verify(char*& str)
{
	Lits_t org;
	uint32 v = 0, s = 0, saved = 0;
	bool clauseSAT = false;
	while ((v = toInteger(str, s)) != 0) {
		if (v > orgVars) {
			LOGERRN("too many variables");
			return false;
		}
		orgLiterals++;
		uint32 lit = V2DEC(v, s);
		LIT_ST marker = marks[v];
		if (UNASSIGNED(marker)) {
			marks[v] = SIGN(lit);
			org.push(lit);
		}
		else if (marker != SIGN(lit)) {
			saved = lit;
			clauseSAT = true;
			break;
		}
	}
	if (!clauseSAT) {
		forall_clause(org, k) {
			const uint32 lit = *k;
			if (satisfied(lit)) {
				saved = lit;
				clauseSAT = true;
				break;
			}
		}
	}
	if (clauseSAT) {
		assert(saved);
		LOGN2(4, "  found satisfied literal %-8d in clause", lit2int(saved));
		if (verbose >= 4) printClause(org, false);
	}
	else {
		LOGN2(1, "  no satisfied literals in clause\t");
		if (verbose >= 1) printClause(org, true);
	}
	forall_clause(org, k) {
		marks[ABS(*k)] = UNDEF_VAL;
	}
	org.clear(true);
	return clauseSAT;
}
