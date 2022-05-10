﻿/***********************************************************************[main.cpp]
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

/*
		░██████╗███████╗░██████╗░███████╗██████╗░░█████╗░░██████╗████████╗
		██╔════╝██╔════╝██╔═══██╗██╔════╝██╔══██╗██╔══██╗██╔════╝╚══██╔══╝
		╚█████╗░█████╗░░██║██╗██║█████╗░░██████╔╝██║░░██║╚█████╗░░░░██║░░░
		░╚═══██╗██╔══╝░░╚██████╔╝██╔══╝░░██╔══██╗██║░░██║░╚═══██╗░░░██║░░░
		██████╔╝███████╗░╚═██╔═╝░██║░░░░░██║░░██║╚█████╔╝██████╔╝░░░██║░░░
		╚═════╝░╚══════╝░░░╚═╝░░░╚═╝░░░░░╚═╝░░╚═╝░╚════╝░╚═════╝░░░░╚═╝░░░
*/

#include "control.hpp"
#include "solve.hpp"
#include "version.hpp"

using namespace SeqFROST;

bool quiet_en       = false;
bool competition_en = false;
int  verbose        = -1;

int main(int argc, char **argv)
{             
	BOOL_OPT opt_competition_en("competition", "engage SAT competition mode", false);
	BOOL_OPT opt_quiet_en("quiet", "enable quiet mode, same as verbose=0", false);
	INT_OPT opt_verbose("verbose", "set the verbosity", 1, INT32R(0, 4));
	INT_OPT opt_timeout("timeout", "set out-of-time limit in seconds", 0, INT32R(0, INT32_MAX));
	INT_OPT opt_memoryout("memoryout", "set out-of-memory limit in gigabytes", 0, INT32R(0, 256));
	if (argc == 1) LOGERR("no input file specified");
	try {
		parseArguments(argc, argv);
		competition_en = opt_competition_en;
		quiet_en = opt_quiet_en, verbose = opt_verbose;
		if (quiet_en || competition_en) verbose = 0;
		else if (!verbose || competition_en) quiet_en = true;
		if (!quiet_en && verbose) {
			LOGTITLE("SeqFROST (Sequential Formal Reasoning On Satisfiability)", version());
			LOGAUTHORS("Muhammad Osama Mahmoud");
			LOGRIGHTS("Technische Universiteit Eindhoven (TU/e)");
			LOGRULER('-', RULELEN);
			if (argc > 2) {
				LOGN0(" Embedded options: ");
				for (int i = 0, j = 0; i < options.size(); ++i) {
					if (options[i]->isParsed()) {
						options[i]->printArgument();
						if (++j % 4 == 0) { PUTCH('\n'); LOGN0("\t\t      "); }
					}
				}	
				SETCOLOR(CNORMAL, stdout);
				PUTCH('\n'); 
				LOGRULER('-', RULELEN);
			}
		}
		signal_handler(handler_terminate);
		string formula = argv[1];
		Solver* seqfrost = new Solver(formula);
		solver = seqfrost;
		if (opt_timeout > 0) set_timeout(opt_timeout);
		if (opt_memoryout > 0) set_memoryout(opt_memoryout);
		signal_handler(handler_mercy_interrupt, handler_mercy_timeout);
		seqfrost->solve();
		if (!quiet_en) {
			LOG0("");
			LOGHEADLINE(Exit, CNORMAL);
			LOGN1(" Cleaning up.. ");
		}
		solver = NULL;
		delete seqfrost;
		LOGDONE(1, 5);
		if (!quiet_en) LOGRULER('-', RULELEN);
		return EXIT_SUCCESS;
	}
	catch (std::bad_alloc&) {
		PRINT("%s%s%s", CYELLOW, "ARE YOU SERIOUS NOW?\n", CNORMAL);
		LOGSAT("UNKNOWN");
		return EXIT_FAILURE;
	}
	catch (MEMOUTEXCEPTION&) {
		PRINT("%s%s%s", CYELLOW, "MEMORY OUT\n", CNORMAL);
		LOGSAT("UNKNOWN");
		return EXIT_FAILURE;
	}
}