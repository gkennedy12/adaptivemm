
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
	echo ==============================================================
	cat /etc/default/adaptivemmd
	echo ==============================================================
	echo "If above config not correct, hit <Control-C> to exit and make changes to /etc/default/adaptivemmd"
	echo -n "Else hit <Enter> to continue: "
	read ANS
}

usage()
{
	echo "usage: $0 <-d sample_directory> [-l <max_loops>] [-m <maxgap>] [-a][-C][-R][-V][-h]" ; exit ;
}

CAPTURE=false
REPLAY=false
VALIDATE=false
MAXGAP=0

MAX_LOOPS=10

while getopts aCRVd:l:m:h name
do
	case $name in
	a)	CAPTURE=true ; REPLAY=true ; VALIDATE=true ;;
	C)	CAPTURE=true ;;
	R)	REPLAY=true ;;
	V)	VALIDATE=true ;;
	d)	DIR="$OPTARG" ;;
	l)	MAX_LOOPS="$OPTARG" ;;
	m)	MAXGAP="$OPTARG" ;;
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

if $CAPTURE || $REPLAY ; then
	setup
fi

if $CAPTURE ; then
	echo
	echo "Run adaptivemmd and capture data.."
	echo

	if [ -d "$DIR" ] ; then
		echo -n "Capture directory ($DIR) cannot exist to avoid overwrite. Hit <Enter> to remove it: "
		read ANS
		rm -rf $DIR
	fi
	rm -f /tmp/capture.out

	echo -n "Current value of /proc/sys/vm/watermark_scale_factor: "
	cat /proc/sys/vm/watermark_scale_factor

	echo
	echo -n "Enter new value (max 1000) or just <Enter> to continue: "
	read M
	if [ ! -z "$M" ] ; then
		if [ $M -gt 1000 ] ; then
			echo "Value entered is too large! Setting value to 1000."
			M=1000
		fi
		echo $M > /proc/sys/vm/watermark_scale_factor
		cat /proc/sys/vm/watermark_scale_factor
	fi

	pkill adaptivemmd
	rm -f /var/run/adaptivemmd.pid
	ps -ed |grep adaptivemmd

	CMD="../../adaptivemmd -a 3 -d -vvvvv -C -m $MAXGAP -l $MAX_LOOPS -S $DIR"
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

	rm -f /tmp/replay.out
	
	if [ -d /tmp/adaptivemmd_data_capture_and_validate_REPLAY ] ; then
		rm -rf /tmp/adaptivemmd_data_capture_and_validate_REPLAY
	fi
	cp -rp $DIR /tmp/adaptivemmd_data_capture_and_validate_REPLAY

	pkill adaptivemmd
	rm -f /var/run/adaptivemmd.pid
	ps -ed |grep adaptivemmd

	CMD="../../adaptivemmd -a 3 -d -vvvvv -R -m $MAXGAP -l $MAX_LOOPS -S /tmp/adaptivemmd_data_capture_and_validate_REPLAY"
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
