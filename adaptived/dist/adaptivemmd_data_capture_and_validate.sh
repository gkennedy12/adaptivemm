
if [ ! -f ../../adaptivemmd ] ; then
	echo "Cannot find ../../adaptivemmd"
	exit
fi

setup()
{
	echo "Checking adaptivemmd config file in /etc/default..."
	if [ ! -f /etc/default/adaptivemmd ] ; then
		echo "/etc/default/adaptivemmd missing. Must setup first."
		exit
	fi
	grep ENABLE /etc/default/adaptivemmd
	echo -n "Are the ENABLEs setup correctly? Hit <Enter> to continue or exit and edit /etc/default/adaptivemmd: "
	read ANS

	pkill adaptivemmd
	rm -f /var/run/adaptivemmd.pid
	ps -ed |grep adaptivemmd
}

usage()
{
	echo "usage: $0 <-d sample_directory> [-m <max_loops] [-a][-C][-R][-V][-h]" ; exit ;
}

CAPTURE=false
REPLAY=false
VALIDATE=false

MAX_LOOPS=10

while getopts aCRVd:m:h name
do
	case $name in
	a)	CAPTURE=true ; REPLAY=true ; VALIDATE=true ;;
	C)	CAPTURE=true ;;
	R)	REPLAY=true ;;
	V)	VALIDATE=true ;;
	d)	DIR="$OPTARG" ;;
	m)	MAX_LOOPS="$OPTARG" ;;
	h)	usage ;;
	*)	usage ;;
	esac
done

if [ -z "$DIR" ] ; then
	usage
fi

if ! $CAPTURE && ! $REPLAY && ! $VALIDATE ; then
	usage
fi

if $CAPTURE ; then
	if [ -d $DIR ] ; then
		echo "Capture directory cannot exist to avoid overwrite."
		exit
	fi
fi

if $CAPTURE ; then
	echo
	echo "Run adaptivemmd and capture data.."
	echo

	setup

	rm /tmp/capture.out

	CMD="../../adaptivemmd -a 3 -d -vvvvv -C -l $MAX_LOOPS -S $DIR"
	echo $CMD
	$CMD 2>&1 |tee /tmp/capture.out
fi
if [ ! -d $DIR ] ; then
	echo "Cannot find Capture directory with previously captured sample data."
	exit
fi
if $REPLAY ; then
	echo
	echo "Run adaptivemmd and replay captured data.."
	echo

	setup

	rm /tmp/replay.out

	CMD="../../adaptivemmd -a 3 -d -vvvvv -R -l $MAX_LOOPS -S $DIR"
	echo $CMD
	$CMD 2>&1 |tee /tmp/replay.out
fi
if ! $VALIDATE ; then exit ; fi

echo
echo "Validate adaptivemmd captured data.."
echo
grep "reclamation rate" /tmp/capture.out /tmp/replay.out >/dev/null 2>&1
if [ $? = 0 ] ; then
	echo "NOTE: ignore any \"reclamation rate\" diffs (they are timing diffs between capture & replay)."
	echo
fi
set -x
diff /tmp/capture.out /tmp/replay.out
