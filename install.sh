#!/bin/bash

ch='|'
lineWidth=90
logfile=install.log
maketemplate=templates/makefile.in
binary=seqfrost
library=lib${binary}.a
currdir=$PWD

echo -n > $logfile

# general functions
usage () {
printf "+%${lineWidth}s+\n" |tr ' ' '-'
cat << EOF
$ch usage: install [ <option> ... ]
$ch 
$ch where '<option>' is one of the following
$ch
$ch	-h or --help          print this usage summary
$ch	-n or --less          print less verbose messages
$ch	-q or --quiet         be quiet (make steps still be saved in the log)
$ch	-c or --clean         remove old installation
$ch	-t or --assert        enable only code assertions
$ch	-l or --logging       enable logging (needed for verbosity level > 2)
$ch	-s or --statistics    enable costly statistics (may impact runtime)
$ch	-a or --all           enable all above flags except 'debug', 'clean' and 'verbosity'
$ch	-p or --pedantic      compile with '-pedantic' flag
$ch	-f or --fast          compile with '-use_fast_math' flag
$ch	-w or --wall          compile with '-Wall' flag
$ch	-d or --debug         compile with debugging information
$ch	--standard=<n>        compile with <11 | 14 | 17> c++ standard
$ch	--extra="flags"       pass extra "flags" to the compiler(s)
EOF
printf "+%${lineWidth}s+\n" |tr ' ' '-'
exit 0
}

error () {
printf "$ch error: %-$((lineWidth - 1))s\n" "$1"
printf "$ch %${lineWidth}s\n" |tr ' ' '-'
exit 1
}

#---------
# options
#---------
noverb=0
quiet=0
all=0
wall=1
fast=0
debug=0
extra=""
clean=0
assert=0
logging=0
pedantic=0
standard=17
statistics=0

while [ $# -gt 0 ]
do
  case $1 in

   	-h|--help) usage;;
	-n|--less) noverb=1;;
	-q|--quiet) quiet=1;;	
	-c|--clean) clean=1;;
	
	-f|--fast) fast=1;;
	-w|--wall) wall=1;;
   	-d|--debug) debug=1;;
   	-t|--assert) assert=1;;
	-p|--pedantic) pedantic=1;;
	
    -l|--logging) logging=1;;
	-s|--statistics) statistics=1;;

	-a|--all) all=1;;

    --standard=*)
      standard="${1#*=}"
      ;;

    --extra=*)
      extra="${1#*=}"
      ;;

    *) error "invalid option '$1' (use '-h' for help)";;

  esac
  shift
done

if [ $debug = 1 ] && [ $assert = 1 ]; then error "cannot combine 'assert' and 'debug' modes"; fi

if [ $clean = 1 ] && [ $all = 1 ]; then error "cannot combine 'clean' and 'all' flags"; fi

if [ $noverb = 1 ] && [ $quiet = 1 ]; then error "cannot combine 'less' and 'quiet' flags"; fi

if [[ ($noverb == 1 || $quiet == 1) && $all == 1 ]]; then error "cannot combine 'less|quiet' and 'all' flags"; fi

if [ ! $standard = 11 ] && [ ! $standard = 14 ] && [ ! $standard = 17 ]; then 
	error "invalid c++ standard '$standard'"
fi

if [ $all = 1 ]; then wall=1;fast=1;debug=0;assert=0;pedantic=1;logging=1;statistics=1; fi

pfbanner () {
if [ $noverb = 0 ] && [ $quiet = 0 ]; then
	printf "+%${lineWidth}s+\n" |tr ' ' '-'
	printf "$ch %-$((lineWidth - 1))s$ch\n"  \
	"                    SeqFROST solver installer (use -h for options)"
	printf "+%${lineWidth}s+\n" |tr ' ' '-'
fi
}

pffinal () {
if [ $noverb = 0 ] && [ $quiet = 0 ]; then
	log ""
	log "check '$1' directory for '$binary' and its library '$library'" 
	printf "+%${lineWidth}s+\n" |tr ' ' '-'
fi
}

ruler () {
if [ $noverb = 0 ] && [ $quiet = 0 ]; then
	echo -n $ch
	printf "%${lineWidth}s+\n" |tr ' ' '-'
	echo -n $ch >> $logfile
	printf "%${lineWidth}s+\n" |tr ' ' '-' >> $logfile
fi
}

log () {
if [ $noverb = 0 ] && [ $quiet = 0 ]; then
	printf "$ch %-$((lineWidth - 1))s\n" "$1"
	printf "$ch %-$((lineWidth - 1))s\n" "$1" >> $logfile
fi
}

logn () {
if [ $noverb = 0 ] && [ $quiet = 0 ]; then
	printf "$ch %s" "$1"
	printf "$ch %s" "$1" >> $logfile
fi
}

endline () {
if [ $noverb = 0 ] && [ $quiet = 0 ]; then
	echo "done."
	echo "done." >> $logfile
fi
}

# banner
pfbanner

# cleaning
if [ $clean = 1 ]; then 
	logn "cleaning up solver installation (other options will be ignored).."
	rm -rf build
	rm -f Makefile
	srcdir=src
	rm -f $srcdir/Makefile
	rm -f $srcdir/*.o $srcdir/$binary $srcdir/$library
	endline
	ruler
	exit 0
fi

# operating system
HOST_OS=$(uname -srmn 2>/dev/null | tr "[:upper:]" "[:lower:]")
[ -z "$HOST_OS" ] && error "cannot communicate with the operating system"

# host compiler
HOST_COMPILER=g++

# compiler version
compilerVer=$(g++ --version | sed -n '1p')
compilerVer=$(echo $compilerVer|tr -d '\n')
compilerVer=$(echo $compilerVer|tr -d '\r')
[ -z "$compilerVer" ] && error "cannot read the compiler version"

# target arch
TARGET_ARCH=$(uname -m)
[ -z "$TARGET_ARCH" ] && error "cannot read the system architecture"

# target size
TARGET_SIZE=$(getconf LONG_BIT)
[ -z "$TARGET_SIZE" ] && error "cannot read the architecture bit-size"

# time
now=$(date)
[ -z "$now" ] && error "cannot read the system date"

# generate version header file without compiler verions
vertemplate=templates/version.in
[ ! -f $vertemplate ] && error "cannot find '$vertemplate' template file"
versionme () {
	[ ! -f $2 ] && error "cannot find local '$2' header"
	line=$(echo $1|tr -d '\n')
	line=$(echo $line|tr -d '\r')
	version=$(echo $line| cut -d':' -f 2)
	[ ! -z "$version" ] && echo "#define VERSION \"$version\"" >> $2
	echo "#define OSYSTEM \"$HOST_OS\"" >> $2
	echo "#define DATE \"$now\"" >> $2
}
solverbuild=src/version.hpp
cp $vertemplate $solverbuild
logn "generating header 'version.hpp' from 'version.in'.."
solverversion=0
[ -f VERSION ] && solverversion=$(sed -n '1p' < VERSION)
versionme "$solverversion" "$solverbuild"
endline; log ""

#------------------------
# start building solver
#------------------------

srcdir=src
builddir=build
makefile=$srcdir/Makefile

# default flags
OPTIMIZE="-O3"
ARCH="-m${TARGET_SIZE}"
STD="-std=c++$standard"

log "installing SeqFROST on '$now'"
log " under operating system '$HOST_OS'"
log " with a '$compilerVer' compiler"
log ""

[ ! -f $solverbuild ] && error "cannot find '$solverbuild' generated file"
echo "#define COMPILER \"$compilerVer\"" >> $solverbuild

logn "creating '$HOST_COMPILER' flags.."

if [ $debug = 0 ] && [ $assert = 0 ]; then 
	CCFLAGS="$CCFLAGS -DNDEBUG $OPTIMIZE"
elif [ $debug = 1 ]; then
	CCFLAGS="$CCFLAGS -g"
elif [ $assert = 1 ]; then 
	CCFLAGS="$CCFLAGS $OPTIMIZE"
fi
if [[ "$HOST_OS" == *"cygwin"* ]]; then pedantic=0; fi
[ $wall = 1 ] && CCFLAGS="$CCFLAGS -Wall"
[ $pedantic = 1 ] && CCFLAGS="$CCFLAGS -pedantic"
[ $fast = 1 ] && CCFLAGS="$CCFLAGS -use_fast_math"
[ $logging = 1 ] && CCFLAGS="$CCFLAGS -DLOGGING"
[ $statistics = 1 ] && CCFLAGS="$CCFLAGS -DSTATISTICS"

CCFLAGS="$ARCH $STD$CCFLAGS"

if [[ $extra != "" ]]; then CCFLAGS="$CCFLAGS $extra"; fi

endline

# building

log ""
log "building with:"
log ""
log "'$CCFLAGS'"
log ""

[ ! -f $maketemplate ] && error "cannot find makefile template"

cp $maketemplate $makefile
sed -i "s|^CCFLAGS.*|CCFLAGS := $CCFLAGS|" $makefile
sed -i "s/^BIN.*/BIN := $binary/" $makefile
sed -i "s/^LIB.*/LIB := $library/" $makefile

log ""

mkdir -p $builddir

cd $srcdir
if [ $quiet = 0 ]; then make; else make >> $logfile; fi
cd $currdir

if [ ! -f $srcdir/$binary ] || [ ! -f $srcdir/$library ]; then
	log ""
	error "could not install the solver due to previous errors"
fi
mv $srcdir/$binary $builddir
mv $srcdir/$library $builddir

pffinal $builddir
