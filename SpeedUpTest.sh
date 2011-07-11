#!/bin/sh
#Copyright UVSQ, 2009, under GPL
#Authors : Sebastien Briais and Sid-Ahmed-Ali Touati
usage() {
    echo "`basename $0` config.csv [--conf-level #value] [--weight custom|equal|fraction] [--precision #value] [-o outputprefix]"
}

check_arg() {
    if test "$1" -eq 0 
    then
	usage
	exit 1
    fi
}

erase_file() {
    if test -s "$1"
    then
	echo -n "$1 already exists: are you sure to continue? "
	read ans
	case "$ans" in
	    y|Y|yes|YES)
		rm -f "$1"
		;;
	    *)
		exit 0
		;;
	esac
    fi
}

get_location() {
    if test -L "$1"
    then
	get_location `readlink "$1"`
    elif test -r "$1"
    then
	dirname "$1"
    else
	echo "Unrecoverable error. Please submit a bug report."
	exit 1
    fi
}

check_arg "$#"

idir=`dirname "$1"`
ifile=`basename "$1"`
shift

if ! (test -s "$idir/$ifile")
then
    echo "$ifile is empty."
    exit 1
fi

if ! (test -r "$idir/$ifile")
then
    echo "$ifile is not readable."
    exit 1
fi

ofile="$ifile".out
oRfile="$ifile".report
eRfile="$ifile".error
wRfile="$ifile".warning

ifile="$idir/$ifile"
ofile="$idir/$ofile"
oRfile="$idir/$oRfile"
eRfile="$idir/$eRfile"
wRfile="$idir/$wRfile"

overwrite=1

ARGS=""

while test "$#" -ne 0
do
    case "$1" in
	--conf-level)
	    shift
	    check_arg "$#"
	    ARGS="$ARGS"' config.level='"$1"
	    ;;
	--weight)
	    shift
	    check_arg "$#"
	    case "$1" in
		equal)
		    ARGS="$ARGS"' config.weight="equal"'
		    ;;
		fraction)
		    ARGS="$ARGS"' config.weight="fraction"'
		    ;;
		custom)
		    ARGS="$ARGS"' config.weight="custom"'
		    ;;
		*)
		    echo "unknown weight parameter"
		    usage
		    exit 1
	    esac
	    ;;
	--precision)
	    shift
	    check_arg "$#"
	    ARGS="$ARGS"' config.precision='"$1"
	    ;;
	-o)
	    shift
	    check_arg "$#"
	    oRfile="$1.report"
	    eRfile="$1.error"
	    wRfile="$1.warning"
	    ofile="$1.out"
	    ;;
	*)
	    echo "Unknown argument $1"
	    usage
	    exit 1
    esac
    shift
done

if test "$overwrite" -eq 0
then
    erase_file "$ofile"
    erase_file "$oRfile"
    erase_file "$eRfile"
    erase_file "$wRfile"
fi

ARGS="$ARGS"' config.ifile="'"$ifile"'"'
ARGS="$ARGS"' config.ofile="'"$ofile"'"'
ARGS="$ARGS"' config.rfile="'"$oRfile"'"'
ARGS="$ARGS"' config.wfile="'"$wRfile"'"'

#echo $ARGS

dir=`get_location "$0"`

if test `expr "$dir" : "^/"` -eq 0
then
    dir=`pwd`"/$dir"
fi

scriptR="$dir"/SpeedUpTest.R

if ! (test -s "$scriptR")
then
    echo "Error: R script not found."
    exit 1
fi

Rcmd=`which R 2> /dev/null`
if test "$?" -ne 0
then
    echo "R command not found."
    exit 1
fi
if ! (test -x "$Rcmd")
then
    echo "R command ($Rcmd) not executable."
    exit 1
fi

#cpath=`pwd`
#cd "$idir"
"$Rcmd" CMD BATCH --no-save --no-restore --slave '--args '"$ARGS" "$scriptR" "$eRfile"
if test "$?" -ne 0
then
    echo "An error occurred while processing."
    echo "Please check $eRfile for more details."
    exit 1
else
    cat "$oRfile"
fi
#cd "$cpath"
