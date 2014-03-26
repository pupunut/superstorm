#! /bin/bash

ulimit -s unlimited
ulimit -c unlimited

function ERR
{
    dolog "ERROR $@"
    return 1
}

function DIE
{
    dolog "FATAL ERROR: $@"
    exit 1
}

function INFO
{
    dolog "$@"
}

function CMD
{
    dolog "execute: $@: begin"

    #NOTE: set -o pipefail is a must, or the return value is the last command in
    #the pipe, here is the return value of tee -a $log
    local cmd
    if type $1 2>&1 |grep -q "function"; then
        cmd="$@"
    else
        cmd="$@ 2>&1 | tee -a $log"
    fi

    if eval $cmd; then
        dolog "execute: $@: success"
        return 0
    else
        local ret=$?
        dolog "execute: $@: failed, ret:$ret"
        return $ret
    fi
}

function dolog
{
    local d=$(date +"%F %T")
    echo -e "$d ==> $@" 2>&1 | tee -a $log
}

function isdigit
{
    [[ $# -eq 1 ]] || return 1

    case $1 in
        *[!0-9]*|"") return 1 ;;
    *) return 0 ;;
esac
}

function isdigit2
{
    [[ $# -eq 1 ]] || return 1

    case $1 in
        *[!0-9.]*|"") return 1 ;;
    *) return 0 ;;
esac
}


function isalpha
{
    [[ $# -eq 1 ]] || return 1

    case $1 in
        *[!a-zA-Z]*|"") return 1 ;;
    *) return 0 ;;
esac
}

function get_array
{
    echo $@
}

function get_array_size
{
    echo $#
}

function isdate
{
    CMD isdigit $1 || return 1

    y=${1:0:4}
    m=${1:4:2}
    d=${1:6:2}

    if [ $y -lt 2000 ] || [ $y -gt $(date +%Y) ]; then
        return 1
    fi

    if [ $m -lt 1 ] || [ $m -gt 12 ]; then
        return 1
    fi

    if [ $d -lt 1 ] || [ $m -gt 31 ]; then
        return 1
    fi

    return 0
}

function create_report
{
sqlite3 $db << EOF
drop table if exists report;
create table report(total integer, up integer, succ integer, coarse integer, fine integer, policy integer, avgp integer, maxp integer, minp integer, avgp1 integer);
insert into report select count(*), 0, 0, coarse, fine, policy, cast(avg(pp) as integer), max(pp), min(pp), 0 from view_sp where fine>0 group by coarse, fine, policy;
update report set up=(select count(*) from view_sp where coarse=report.coarse and fine=report.fine and policy=report.policy and pp > 0 group by
coarse,fine,policy);
update report set avgp1=(select cast(avg(pp) as integer) from view_sp where coarse=report.coarse and fine=report.fine and policy=report.policy and pp > 0 group by coarse,fine,policy);
update report set succ=up*100/total;
EOF

}

function view_report
{
    echo "==================="
    echo "statistics"
    echo "==================="

sqlite3 $db << EOF
.header on
.mode column
select * from report;
EOF

cat << --EOF-- >> /dev/null
sqlite3 $db << EOF
.header on
.mode column
select * from view_sp where pp<0 and fine>0 order by pp ASC limit 50;
select * from view_sp where pp>100 and fine>0 order by pp DESC limit 50;
select * from point where date = max(date) and fine>0;
EOF
--EOF--
}

function view_bpn
{
    echo "==================="
    echo "bpn"
    echo "==================="

    local bpn=$(sqlite3 $db "select sn from bpn;")
    for i in $(echo $bpn); do printf %06d\\n $i; done
}

function env_init()
{
    #create log dir
    if [ -z "$logdir" ]; then
        LOGDIR="/tmp/$(basename $0 .sh)"
        logdir="$LOGDIR"
        mkdir -p $logdir || DIE "Failed to mkdir $logdir"

    fi
    #log="$logdir/log_$(date +%y%m%d_%H_%M%S)"
    log="$logdir/log"

    tmpfile="/tmp/$(basename $0 .sh).tmp"

    find_low="${0%/*}/find_low"
    find_bpn="${0%/*}/find_bpn"
    eval_bp="${0%/*}/eval_bp"

    if [ ! -x "$find_low" ]; then
        find_low=${find_low##*/}
        find_low=$(which $find_low) || DIE "Cannot find $find_low"
    fi

    if [ ! -x "$find_bpn" ]; then
        find_bpn=${find_bpn##*/}
        find_bpn=$(which $find_bpn) || DIE "Cannot find $find_bpn"
    fi


    if [ ! -x "$eval_bp" ]; then
        eval_bp=${eval_bp##*/}
        eval_bp=$(which $eval_bp) || DIE "Cannot find $eval_bp"
    fi

    [[ ! -x "sqlite3" ]] || DIE "Cannot find sqlite3"
}

function usage()
{
    echo "Usage:"
    echo "	#1) ${PROGNAME} -d db -y year [-m month] [-v]"
    echo "	#2) ${PROGNAME} -d db -a head-day -t tail-day"
    echo "	#3) ${PROGNAME} -d db -n -t tail-day"
    echo "	#4) ${PROGNAME} -h"
}

function helptext()
{
        cat <<- -EOF-

        ${FULLNAME} ver. ${VERSION}
        A Tool to run find and eval in period of month or year

        $(usage)

        [Usage #1]
        Run find and eval in some year or some month.
        ${PROGNAME} -d db -y year [-m month]
        -d db       \$db is full path of dayline db
        -y year     \$year as is
        -m month    \$month as is
        -v          Print detailed message on stdout.


        [Usage #2]
        Run find and eval between two days.
        ${PROGNAME} -d db -a head-day -t tail-day
        -d db       \$db is full path of dayline db
        -a head-day \$head-day as is
        -t tail-day \$tail-day as is

        [Usage #3]
        Run find and eval between two days.
        ${PROGNAME} -d db -n -t tail-day
        -d db       \$db is full path of dayline db
        -t tail-day \$tail-day as is
        -n          find bpn flag

        [Usage #4]
        Display this help message and exit.
        ${PROGNAME} -h
-EOF-
}

#BEGIN...

PROGNAME=$(basename $0)
FULLNAME="REPORTOR"

verbose_flag=""
db=""
year=""
month=""
head_day=""
tail_day=""
param=""
bpn_flag=""

if [ "$#" -eq 0 ]; then
	usage
	exit 0
fi

while getopts "d:y:m:a:t:nvh" opt; do
        case $opt in
		a )     head_day=${OPTARG}
			;;
                d )     db=${OPTARG}
                        ;;
                m )     month="${OPTARG}"
                        ;;
                n )     bpn_flag=true;
                        ;;
                t )     tail_day="${OPTARG}"
                        ;;
                y )     year="${OPTARG}"
                        ;;
		v )     verbose_flag=true
			;;
                h )     helptext
                        exit 0
                        ;;
   		:)	usage
			exit 1
			;;
    		?)	usage
        		exit 1
        		;;
                * )     usage
                        exit 2
                        ;;
        esac
done
shift $(($OPTIND - 1))

#set
set -o pipefail

#initialize environment
env_init

#check parameters
[[ -n "$db" ]] || DIE "db:$db is NULL"
[[ -w "$db" ]] || DIE "No r/w permission on db:$db" 
param="-d${db}"

if [ -n "$year" ]; then
    if [ $year -lt 2000 ] || [ $year -gt $(date +%Y) ]; then
        DIE "Invalid year:$year, should between 2000 and $(date +%Y)"
    fi
    if [ -n "$head_day" ] || [ -n "$tail_day" ]; then
        DIE "Mixed usage, please see help with ${PROGNAME} -h"
    fi
    [[ -n "month" ]] || param="${param} -a${year}0101 -t${year}1231"
fi

if [ -n "$month" ]; then
    if [ $month -lt 1 ] || [ $month -gt 12 ]; then
        DIE "Invalid month:$month, should between 1 and 12"
    fi
    if [ $month -ge 1 ] && [ $month -le 9 ]; then
        month="0$month"
    fi
    [[ -n "$year" ]] || DIE "Should provide year if month is provided"
    param="${param} -a${year}${month}01 -t${year}${month}31"
fi

if [ -n "$head_day" ]; then
    [[ -n "$tail_day" ]] || DIE "Should provide tail_day"
    CMD isdate $head_day || DIE "Invalid format or date of head_day:$head_day"
    param="${param} -a${head_day}"
fi

if [ -n "$tail_day" ]; then
    if [ -n "bpn_flag" ]; then
        [[ -n "$head_day" ]] || DIE "Should provide head_day"
    fi
    CMD isdate $tail_day || DIE "Invalid format or date of tail_day:$tail_day"
    param="${param} -t${tail_day}"
fi

if [ -n "bpn_flag" ]; then
    CMD $find_bpn $param || DIE "Failed to run $find_bpn $param" 
    CMD view_bpn || DIE "Failed to view_bpn"
else
    CMD $find_low $param || DIE "Failed to run $find_low $param" 
    CMD $eval_bp -d $db || DIE "Failed to run $eval_bp -d $db"
    CMD create_report || DIE "Failed to create_report"
    CMD view_report || DIE "Failed to view_report"
fi


