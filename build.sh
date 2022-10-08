#!/bin/bash

# set -exu

usage() {
    echo "Usage: bash $0 -d <device> -p <product name> -t <release version> -v <build variant> -c <container type: docker or lxc> -j <make -j Num>"
    echo "For example, "
    echo "    $ bash $0 -p aosp_arm64 -v user -j 10"
    exit 1
}

# parse the parameters
BUILD_PRODUCT_NAME="aosp_arm64"
BUILD_VARIANT="user"
BUILD_MAKE_J=10

while getopts "d:p:v:c:j:t:n" option
do
  case "$option" in
    p)
      BUILD_PRODUCT_NAME=${OPTARG}
      ;;
    v)
      BUILD_VARIANT=${OPTARG}
      ;;
    j)
      BUILD_MAKE_J=${OPTARG}
      ;;
    *)
      usage
      ;;
  esac
done

CMD_REAL_PATH=`readlink -e "$0"`
if [ $? -ne 0 ]; then
    CMD_REAL_PATH="$0"
fi

AOSP_ROOT=`dirname "$CMD_REAL_PATH"`
cd $AOSP_ROOT

set u

source $AOSP_ROOT/build/envsetup.sh
lunch $BUILD_PRODUCT_NAME-$BUILD_VARIANT

make -j $BUILD_MAKE_J