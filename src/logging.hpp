/***********************************************************************[logging.hpp]
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

#ifndef __LOGGING_
#define __LOGGING_

#include <cstdio>
#include <cstring>
#include "constants.hpp"
#include "color.hpp"

#define RULELEN     92
#define PREFIX      "c "
#define UNDERLINE	"\u001b[4m"

#if defined(__linux__) || defined(__CYGWIN__)
#pragma GCC system_header
#endif

#define PUTCH(CH) putc(CH, stdout);

#define PRINT(FORMAT, ...) fprintf(stdout, FORMAT, ## __VA_ARGS__);

inline void REPCH(const char& ch, const size_t& size, const size_t& off = 0) {
    for (size_t i = off; i < size; ++i) PUTCH(ch);
}

#define LOGSAT(RESULT) \
  do { \
     if (!quiet_en) LOG0(""); \
     PRINT("s %s\n", RESULT); \
  } while (0)

#define LOGERR(FORMAT, ...) \
  do { \
     SETCOLOR(CERROR, stderr); \
     fprintf(stderr, "ERROR - "); \
     fprintf(stderr, FORMAT, ## __VA_ARGS__); \
     putc('\n', stderr); \
     SETCOLOR(CNORMAL, stderr); \
     exit(1); \
  } while (0)

#define LOGERRN(FORMAT, ...) \
  do { \
     SETCOLOR(CERROR, stderr); \
     fprintf(stderr, "ERROR - "); \
     fprintf(stderr, FORMAT, ## __VA_ARGS__); \
     putc('\n', stderr); \
     SETCOLOR(CNORMAL, stderr); \
  } while (0)

#define LOGWARN(FORMAT, ...) \
  do { \
     SETCOLOR(CWARNING, stderr); \
     fprintf(stderr, "WARNING - ");\
     fprintf(stderr, FORMAT, ## __VA_ARGS__);\
     PUTCH('\n'); \
     SETCOLOR(CNORMAL, stderr); \
  } while (0)

#define LOGRULER(CH, TIMES) \
  do { \
     PUTCH(PREFIX[0]); \
     REPCH(CH, TIMES);      \
     PUTCH('\n'); \
  } while (0)

#define LOGHEADLINE(HEAD, COLOR) \
  do { \
     const char* HEADSTR = "[ " #HEAD " ]"; \
	 size_t len = strlen(HEADSTR); \
	 if (RULELEN < len) LOGERR("ruler length is smaller than header line"); \
	 size_t gap = (RULELEN - len) / 2; \
	 PUTCH('c'); \
	 REPCH('-', gap); \
	 PRINT("%s%s%s", COLOR, HEADSTR, CNORMAL); \
	 REPCH('-', gap); \
	 PUTCH('\n'); \
  } while (0); \

#define LOGTITLE(NAME, VER) \
  do { \
     LOGRULER('-', RULELEN); \
     const char* suffix = "SAT Solver (version "; \
     size_t len = strlen(NAME) + strlen(suffix) + strlen(VER) + 1; \
     if (RULELEN < len) LOGERR("ruler length is smaller than the title"); \
     size_t gap = (RULELEN - len - 3) / 2; \
     PRINT(PREFIX); PUTCH(' '); \
     REPCH(' ', gap); \
     PRINT("%s%s%s %s%s%s)%s", UNDERLINE, CSOLVER, NAME, CSOLVER, suffix, VER, CNORMAL); \
     REPCH(' ', RULELEN + 1, len + gap + 3), PUTCH('\n'); \
  } while (0); \

#define LOGAUTHORS(AUTHORS) \
  do { \
     const char *prefix = "Authors: "; \
     size_t len = strlen(prefix) + strlen(AUTHORS); \
     if (RULELEN < len) LOGERR("ruler length is smaller than the authors"); \
     size_t gap = RULELEN - len - 1; \
     PRINT(PREFIX); PUTCH(' '); \
     PRINT("%s%s%s%s", CAUTHOR, prefix, AUTHORS, CNORMAL); \
     REPCH(' ', gap), PUTCH('\n'); \
  } while (0); \

#define LOGRIGHTS(RIGHTS) \
  do { \
     const char *suffix = ", all rights reserved"; \
     size_t len = strlen(RIGHTS) + strlen(suffix); \
     if (RULELEN < len) LOGERR("ruler length is smaller than the rights"); \
     size_t gap = RULELEN - len - 1; \
     PRINT(PREFIX); PUTCH(' ');\
     PRINT("%s%s%s%s", CRIGHTS, RIGHTS, CNORMAL, suffix); \
     REPCH(' ', gap), PUTCH('\n'); \
  } while (0); \

#define LOG0(MESSAGE) do { PRINT(PREFIX); PRINT("%s\n", MESSAGE); } while (0)

#define LOGN0(MESSAGE) do { PRINT(PREFIX); PRINT("%s", MESSAGE); } while (0)

#define LOG1(FORMAT, ...) \
    do { \
        PRINT(PREFIX); PRINT(FORMAT, ## __VA_ARGS__); PUTCH('\n'); \
    } while (0)

#define LOGN1(FORMAT, ...) \
    do { \
        PRINT(PREFIX); PRINT(FORMAT, ## __VA_ARGS__); \
    } while (0)

#define LOG2(VERBOSITY, FORMAT, ...) \
    do { \
        if (verbose >= VERBOSITY) { PRINT(PREFIX); PRINT(FORMAT, ## __VA_ARGS__); PUTCH('\n'); } \
    } while (0)

#define LOGN2(VERBOSITY, FORMAT, ...) \
    do { \
        if (verbose >= VERBOSITY) { PRINT(PREFIX); PRINT(FORMAT, ## __VA_ARGS__); } \
    } while(0)

#define PRINT2(VERBOSITY, MAXVERBOSITY, FORMAT, ...) \
    do { \
        if (verbose >= VERBOSITY && verbose < MAXVERBOSITY) { PRINT(FORMAT, ## __VA_ARGS__); } \
    } while (0)

#define LOGDONE(VERBOSITY, MAXVERBOSITY) if (verbose >= VERBOSITY && verbose < MAXVERBOSITY) PRINT("done.\n");

#define LOGENDING(VERBOSITY, MAXVERBOSITY, FORMAT, ...) \
    do { \
        if (verbose >= VERBOSITY && verbose < MAXVERBOSITY) { \
            PRINT(FORMAT, ## __VA_ARGS__); \
            PRINT(" done.\n"); \
        } \
    } while(0)

#define LOGMEMCALL(SOLVER, VERBOSITY) LOG2(VERBOSITY, " Memory used in %s call = %lld MB", __func__, sysMemUsed() / MBYTE);

#define LOGREDALL(SOLVER, VERBOSITY, MESSAGE) \
    if (verbose >= VERBOSITY) { \
        SOLVER->evalReds(); \
        LOG1("\t\t %s%s%s", CLBLUE, MESSAGE, CNORMAL); \
        SOLVER->logReductions(); }

#define LOGREDCL(SOLVER, VERBOSITY, MESSAGE) \
    if (verbose >= VERBOSITY) { \
        inf.nDeletedVarsAfter = 0; \
        SOLVER->countAll(); \
        LOG1("\t\t %s%s%s", CLBLUE, MESSAGE, CNORMAL); \
        SOLVER->logReductions(); }

#define CNFORGINF(CLS, LITS) \
    uint64 CLS = ORIGINALS; \
    uint64 LITS = ORIGINALLITERALS; \

#define CNFLEARNTINF(CLS, LITS) \
    uint64 CLS = LEARNTS; \
    uint64 LITS = LEARNTLITERALS; \

#define LOGSHRINKALL(VERBOSITY, BCLS, BLITS) \
    do { \
        assert(BCLS >= MAXCLAUSES); \
        assert(BLITS >= MAXLITERALS); \
        uint64 RCLS = BCLS - MAXCLAUSES, RLITS = BLITS - MAXLITERALS; \
        stats.shrink.clauses += RCLS, stats.shrink.literals += RLITS; \
        LOGENDING(VERBOSITY, 5, "(-%lld clauses, -%lld literals)", RCLS, RLITS); \
    } while (0)

#define LOGSHRINKORG(VERBOSITY, BCLS, BLITS) \
    do { \
        assert(BCLS >= ORIGINALS); \
        assert(BLITS >= ORIGINALLITERALS); \
        uint64 RCLS = BCLS - ORIGINALS, RLITS = BLITS - ORIGINALLITERALS; \
        stats.shrink.clauses += RCLS, stats.shrink.literals += RLITS; \
        LOGENDING(VERBOSITY, 5, "(-%lld clauses, -%lld literals)", RCLS, RLITS); \
    } while (0)

#define LOGSHRINKLEARNT(VERBOSITY, BCLS, BLITS) \
    do { \
        assert(BCLS >= LEARNTS); \
        assert(BLITS >= LEARNTLITERALS); \
        uint64 RCLS = BCLS - LEARNTS, RLITS = BLITS - LEARNTLITERALS; \
        stats.shrink.clauses += RCLS, stats.shrink.literals += RLITS; \
        LOGENDING(VERBOSITY, 5, "(-%lld clauses, -%lld literals)", RCLS, RLITS); \
    } while (0)

#ifdef LOGGING

#define LOGBCPS(SOLVER, VERBOSITY, LIT) \
     if (verbose >= VERBOSITY) { \
		LOG1("\t Before BCP(%d)", l2i(LIT)); \
		SOLVER->printWatched(ABS(LIT)); }

#define LOGBCP(SOLVER, VERBOSITY, LIT) \
     if (verbose >= VERBOSITY) { \
		LOG1("\t BCP(%d)", l2i(LIT)); \
		SOLVER->printOL(LIT); \
        SOLVER->printOL(FLIP(LIT)); }

#define LOGTRAIL(SOLVER, VERBOSITY) if (verbose >= VERBOSITY) SOLVER->printTrail();

#define LOGLEARNT(SOLVER, VERBOSITY) if (verbose >= VERBOSITY) SOLVER->printLearnt();

#define LOGSORTED(SOLVER, SIZE, VERBOSITY) if (verbose >= VERBOSITY) SOLVER->printSortedStack(SIZE);

#define LOGCLAUSE(VERBOSITY, CLAUSE, FORMAT, ...) \
    do { \
        if (verbose >= VERBOSITY) { \
            PRINT(PREFIX);\
            PRINT(FORMAT, ## __VA_ARGS__); \
            SETCOLOR(CLOGGING, stdout);\
            CLAUSE.print(); \
            SETCOLOR(CNORMAL, stdout);\
        } \
    } while (0)

#define LOGDL(VERBOSITY) LOG2(VERBOSITY, " Current decision level: %d", LEVEL);

#define LOGBCPE(SOLVER, VERBOSITY, LIT) \
     if (verbose >= VERBOSITY) { \
		LOG1("\t After BCP(%d)", l2i(LIT)); \
		SOLVER->printWatched(ABS(LIT)); \
        LOGRULER('-', 30); }

#define LOGOCCURS(SOLVER, VERBOSITY, VAR) \
     if (verbose >= VERBOSITY) { \
		LOG1("\t Full Occurrence LIST(%d)", VAR); \
		SOLVER->printOccurs(VAR); \
        LOGRULER('-', 30); }

#define LOGNEWLIT(VERBOSITY, SRC, LIT) \
    do { \
        LOG2(VERBOSITY, "   %sNew %s( %d@%d )%s", CREPORTVAL, SRC == UNDEF_REF ? !LEVEL ? "forced unit" : "decision" : "unit", l2i(LIT), l2dl(LIT), CNORMAL); \
    } while (0)

#define LOGCONFLICT(VERBOSITY, LIT) \
    do { \
        LOG2(VERBOSITY, " %sConflict detected in literal( %d@%d )%s", CCONFLICT, l2i(LIT), l2dl(LIT), CNORMAL); \
    } while (0)

#else // NO LOGGING

#define LOGBCPS(SOLVER, VERBOSITY, LIT) do { } while (0)

#define LOGBCP(SOLVER, VERBOSITY, LIT) do { } while (0)

#define LOGTRAIL(SOLVER, VERBOSITY) do { } while (0)

#define LOGLEARNT(SOLVER, VERBOSITY) do { } while (0)

#define LOGSORTED(SOLVER, SIZE, VERBOSITY) do { } while (0)

#define LOGCLAUSE(VERBOSITY, CLAUSE, FORMAT, ...) do { } while (0)

#define LOGDL(VERBOSITY) do { } while (0)

#define LOGBCPE(SOLVER, VERBOSITY, LIT) do { } while (0)

#define LOGOCCURS(SOLVER, VERBOSITY, VAR) do { } while (0)

#define LOGNEWLIT(VERBOSITY, SRC, LIT) do { } while (0)

#define LOGCONFLICT(VERBOSITY, LIT) do { } while (0)
    
#endif // NO LOGGING

#endif