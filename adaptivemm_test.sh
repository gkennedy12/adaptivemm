#!/bin/bash
PATH=$PATH:.
if [ ! -d adaptived/tests/ftests/sample_data ] ; then
	echo "Can't find adaptived/tests/ftests/sample_data"
	exit
fi
if [ ! -f adaptivemmd ] ; then
	echo "Can't find adaptivemmd"
	exit
fi

pkill adaptivemmd
rm -f /var/run/adaptivemmd.pid
ps -ed |grep adaptivemmd

# /work/tom/adaptivemm_fork/adaptived/tests/ftests/sample_data/watermark_logs

pushd adaptived/tests/ftests/sample_data >/dev/null
rm -rf watermark_logs mem_leak_logs
tar xf mem_leak_logs.tar.gz
tar xf watermark_logs.tar.gz
popd >/dev/null

# ======================= Memory Leak Test =======================
rm -f /tmp/replay_mem_leak.log

# sed '/run_memory_checks() failed./d' \

echo "Test adaptivemmd memory leak functionality..."
set -x
adaptivemmd -a 3 -d -vvvvv \
-E 3 \
-S adaptived/tests/ftests/sample_data/mem_leak_logs \
-L /tmp/replay_mem_leak.log 2>&1 | sed '/run_memory_checks/d' |tee /tmp/mem_leak.out
set +x

cp adaptived/tests/ftests/sample_data/mem_leak_logs/adaptivemm.log /tmp/a
cp /tmp/replay_mem_leak.log /tmp/b

sed -i '/adaptivemmd .*verbose.*/d' /tmp/a
sed -i '/adaptivemmd .*verbose.*/d' /tmp/b

sed -i '/^adaptivemm_fopen: can/d' /tmp/b
sed -i '/^adaptivemm_opendir: can/d' /tmp/b

echo
MEM_LEAK_STS=0
diff /tmp/a /tmp/b
if [ $? != 0 ] ; then
	echo "$0 memory leak test failed."
	MEM_LEAK_STS=1
else
	echo "$0 memory leak test passed."
fi
echo

# ======================= Watermark Test =======================
rm -f /tmp/replay_wmark.log

# sed '/run_memory_checks() failed./d' \

echo "Test adaptivemmd watermark functionality..."
set -x
adaptivemmd -a 3 -d -vvvvv \
-E 4 \
-S adaptived/tests/ftests/sample_data/watermark_logs \
-L /tmp/replay_wmark.log 2>&1 | sed '/run_memory_checks/d' |tee /tmp/wmark.out
set +x

cp adaptived/tests/ftests/sample_data/watermark_logs/adaptivemm.log /tmp/a
cp /tmp/replay_wmark.log /tmp/b

sed -i '/adaptivemmd .*verbose.*/d' /tmp/a
sed -i '/adaptivemmd .*verbose.*/d' /tmp/b

sed -i '/^adaptivemm_fopen: can/d' /tmp/b
sed -i '/^adaptivemm_opendir: can/d' /tmp/b
sed -i '/^Failed to open/d' /tmp/b
sed -i '/reclamation rate is/d' /tmp/a
sed -i '/reclamation rate is/d' /tmp/b

sed -i '/^$/d' /tmp/a
sed -i '/^$/d' /tmp/b

if [ ! -f /proc/sys/fs/negative-dentry-limit ] ; then
	sed -i '/Updating negative dentry limit to/d' /tmp/a
fi

echo
WMARK_STS=0
diff /tmp/a /tmp/b
if [ $? != 0 ] ; then
	echo "$0 watermark test failed."
	WMARK_STS=1
else
	echo "$0 watermark test passed."
fi
echo

pushd adaptived/tests/ftests/sample_data >/dev/null
rm -rf watermark_logs mem_leak_logs
popd >/dev/null

if [ $MEM_LEAK_STS != 0 -o $WMARK_STS != 0 ] ; then
	exit 1
fi
exit 0
