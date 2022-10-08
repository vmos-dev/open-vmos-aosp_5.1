#!/bin/sh

rtrn=0

adb shell mount | grep -q /sdcard
if [ 0 -ne $? ]
then
    echo FAILURE: /sdcard is not mounted
    exit 1
fi

for i in nosuid noexec
do
    adb shell mount | grep /sdcard | grep -q $i
    if [ 0 -ne $? ]
    then
        echo FAILURE: /sdcard is not mounted $i
        rtrn=$(expr $rtrn + 1)
    fi
done

for i in mem kmem
do
    adb shell ls /dev/*mem | grep -q $i
    if [ 0 -ne $? ]
    then
        echo FAILURE: $i is present on system
        rtrn=$(expr $rtrn + 1)
    fi

done

exit $rtrn

