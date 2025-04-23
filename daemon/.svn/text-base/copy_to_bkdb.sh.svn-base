#!/bin/sh
if [ $1 -z ]
then
echo "input daemon name "
exit 1
fi

echo "start copy"
scp -r -P 51004 ./rbin/$1 ezwon@61.252.171.36:./zangsi/daemon/bin/.
echo "end copy"
