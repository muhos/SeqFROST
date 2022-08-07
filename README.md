[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Build Status](https://app.travis-ci.com/muhos/SeqFROST.svg?token=YXUywHfBSpqMqyUKnyT4&branch=main)](https://app.travis-ci.com/muhos/SeqFROST)

# SeqFROST
SeqFROST stands for Sequential Formal ReasOning about SaTisfiability. 
It is a sequential SAT solver with inprocessing based on our simplifications engine in GPU SAT solver [ParaFROST](https://github.com/muhos/ParaFROST). Unlike ParaFROST, this solver runs completely single-threaded on the CPU with different solving heuristics. SeqFROST also won the first place in Main Track of SAT Competition 2022 on SAT instances.

# Install

To install, use the `install.sh` script which has the following usage:


&nbsp; usage: `install.sh [ <option> ... ]`<br>
&nbsp; where `<option>` is one of the following

       -h or --help          print this usage summary
       -n or --less          print less verbose messages
       -q or --quiet         be quiet (make steps still be saved in the log)
	   -c or --clean         remove old installation
	   -t or --assert        enable only code assertions
       -l or --logging       enable logging (needed for verbosity level > 2)
       -s or --statistics    enable costly statistics (may impact runtime)
       -a or --all           enable all above flags except 'assert'
	   -w or --wall          compile with '-Wall' flag
	   -f or --fast          compile with '-use_fast_math' flag
       -d or --debug         compile with debugging information
       -p or --pedantic      compile with '-pedantic' flag
       --standard=<n>        compile with <11 | 14 | 17> c++ standard
       --extra="flags"       pass extra "flags" to the compiler(s)


The `seqfrost` binary and the library `libseqfrost.a` will be created by default in the build directory.<br>

## Debug and Testing
Add `-t` argument with the install command to enable assertions or `-d` to collect debugging information.<br>

## Usage
The solver can be used via the command `seqfrost [<infile>.<cnf>][<option> ...]`.<br>
For more options, type `seqfrost -h` or `seqfrost --helpmore`.

# Incremental Solving
SeqFROST supports incremental solving to `add`/`remove` variables or clauses incrementally while solving with assumptions if needed. Thus, the solver can be integrated to any SAT-based bounded model checker.
