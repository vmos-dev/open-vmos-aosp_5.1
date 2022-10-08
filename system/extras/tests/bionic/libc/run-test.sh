#!/bin/sh
#
# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#  This shell script is used to run one test on a device emulator.
#

PROGDIR=`dirname $0`

#
# Parse options
#
VERBOSE=no
VERBOSE2=no
ADB_CMD=adb

while [ -n "$1" ]; do
    opt="$1"
    optarg=`expr "x$opt" : 'x[^=]*=\(.*\)'`
    case "$opt" in
        --help|-h|-\?)
            OPTION_HELP=yes
            ;;
        --verbose)
            if [ "$VERBOSE" = "yes" ] ; then
                VERBOSE2=yes
            else
                VERBOSE=yes
            fi
            ;;
        --adb=*)
            ADB_CMD="$optarg"
            ;;
        -*) # unknown options
            echo "ERROR: Unknown option '$opt', use --help for list of valid ones."
            exit 1
        ;;
        *)  # Simply record parameter
            if [ -z "$PARAMETERS" ] ; then
                PARAMETERS="$opt"
            else
                PARAMETERS="$PARAMETERS $opt"
            fi
            ;;
    esac
    shift
done

if [ "$OPTION_HELP" = "yes" ] ; then
    echo "Usage: $PROGNAME [options] <test-name>"
    echo ""
    echo "Run one C library test on a device/emulator through ADB."
    echo ""
    echo "Valid options:"
    echo ""
    echo "    --help|-h|-?      Print this help"
    echo "    --verbose         Enable verbose mode"
    echo "    --adb=<file>      Specify adb executable for device tests"
    echo ""
    exit 0
fi

if [ -z "$ANDROID_PRODUCT_OUT" ] ; then
    echo "ERROR: ANDROID_PRODUCT_OUT not defined. Please run the 'lunch' command"
    exit 1
fi

if [ ! -f "$ANDROID_PRODUCT_OUT/system.img" ] ; then
    echo "ERROR: Missing file: $ANDROID_PRODUCT_OUT/system.img"
    echo "Are you sure you built the proper system image?"
    exit 1
fi

EXEC_ROOT_PATH="$ANDROID_PRODUCT_OUT/obj/EXECUTABLES"
if [ ! -d "$EXEC_ROOT_PATH" ] ; then
    echo "ERROR: Missing directory: $EXEC_ROOT_PATH"
    echo "Are you sure you built the proper system image?"
    exit 1
fi

if [ -z "$PARAMETERS" ] ; then
    echo "ERROR: Please specify test name."
    echo "Must be one of the following:"
    for FILE in `cd $EXEC_ROOT_PATH && ls -d test_*`; do
        TEST=`echo "$FILE" | sed -e "s!test_\(.*\)_intermediates!\\1!g"`
        echo "  $TEST"
    done
    exit 1
fi

TEST="$PARAMETERS"
# Normalize test name, i.e. remove test_ prefix
TEST=`echo "$TEST" | sed -e "s!^test_!!g"`

TESTDIR="$EXEC_ROOT_PATH/test_${TEST}_intermediates"
if [ ! -d "$TESTDIR" ] ; then
    echo "ERROR: No test by that name: test_$TEST!"
    exit 1
fi

TESTNAME="test_$TEST"
TESTEXE="$TESTDIR/$TESTNAME"
if [ ! -f "$TESTEXE" ] ; then
    echo "ERROR: Missing file: $TESTEXE"
    echo "Are you sure your last test build was complete?"
    exit 1
fi

# Run a command in ADB and return 0 in case of success, or 1 otherwise.
# This is needed because "adb shell" does not return the proper status
# of the launched command.
#
# NOTE: You must call set_adb_cmd_log before that to set the location
#        of the temporary log file that will be used.
#
adb_cmd ()
{
    local RET
    if [ -z "$ADB_CMD_LOG" ] ; then
        dump "INTERNAL ERROR: ADB_CMD_LOG not set!"
        exit 1
    fi
    if [ $VERBOSE = "yes" ] ; then
        echo "$ADB_CMD shell $@"
        $ADB_CMD shell $@ "&&" echo OK "||" echo KO | tee $ADB_CMD_LOG
    else
        $ADB_CMD shell $@ "&&" echo OK "||" echo KO > $ADB_CMD_LOG
    fi
    # Get last line in log, should be OK or KO
    RET=`tail -n1 $ADB_CMD_LOG`
    # Get rid of \r at the end of lines
    RET=`echo "$RET" | sed -e 's![[:cntrl:]]!!g'`
    [ "$RET" = "OK" ]
}

set_adb_cmd_log ()
{
    ADB_CMD_LOG="$1"
}

# Returns 0 if a variable containing one or more items separated
# by spaces contains a given value.
# $1: variable name (e.g. FOO)
# $2: value to test
var_list_contains ()
{
    echo `var_value $1` | tr ' ' '\n' | fgrep -q -e "$2"
}

TMPDIR=/tmp/bionic-tests
mkdir -p $TMPDIR
set_adb_cmd_log $TMPDIR/adb.log.txt

DEVICE_TEST_DIR=/data/local/bionic-test
DEVICE_TEST=$DEVICE_TEST_DIR/$TESTNAME
adb_cmd mkdir $DEVICE_TEST_DIR
$ADB_CMD push $TESTEXE $DEVICE_TEST_DIR/
if [ $? != 0 ] ; then
    echo "ERROR: Can't push test to device!"
    exit 1
fi

adb_cmd chmod 0755 $DEVICE_TEST &&
adb_cmd $DEVICE_TEST
RET=$?
adb_cmd rm -r $DEVICE_TEST_DIR

if [ "$RET" != 0 ] ; then
    echo "FAIL!"
else
    echo "OK!"
fi
exit 0
