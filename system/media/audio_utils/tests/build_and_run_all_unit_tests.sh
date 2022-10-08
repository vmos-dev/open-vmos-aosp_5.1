#!/bin/bash
#
# Run tests in this directory.
#

if [ -z "$ANDROID_BUILD_TOP" ]; then
    echo "Android build environment not set"
    exit -1
fi

# ensure we have mm
. $ANDROID_BUILD_TOP/build/envsetup.sh

mm

echo "waiting for device"

adb root && adb wait-for-device remount

echo "========================================"
echo "testing primitives"
adb push $OUT/system/lib/libaudioutils.so /system/lib
adb push $OUT/system/bin/primitives_tests /system/bin
adb shell /system/bin/primitives_tests
