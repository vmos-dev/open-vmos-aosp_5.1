#!/bin/bash
#
# To call this script, make sure make_ext4fs is somewhere in PATH

function usage() {
cat<<EOT
Usage:
mkuserimg.sh [-s] SRC_DIR OUTPUT_FILE EXT_VARIANT MOUNT_POINT SIZE [-j <journal_size>]
             [-T TIMESTAMP] [-C FS_CONFIG] [-B BLOCK_LIST_FILE] [FILE_CONTEXTS]
EOT
}

ENABLE_SPARSE_IMAGE=
if [ "$1" = "-s" ]; then
  ENABLE_SPARSE_IMAGE="-s"
  shift
fi

if [ $# -lt 5 ]; then
  usage
  exit 1
fi

SRC_DIR=$1
if [ ! -d $SRC_DIR ]; then
  echo "Can not find directory $SRC_DIR!"
  exit 2
fi

OUTPUT_FILE=$2
EXT_VARIANT=$3
MOUNT_POINT=$4
SIZE=$5
shift; shift; shift; shift; shift

JOURNAL_FLAGS=
if [ "$1" = "-j" ]; then
  if [ "$2" = "0" ]; then
    JOURNAL_FLAGS="-J"
  else
    JOURNAL_FLAGS="-j $2"
  fi
  shift; shift
fi

TIMESTAMP=-1
if [[ "$1" == "-T" ]]; then
  TIMESTAMP=$2
  shift; shift
fi

FS_CONFIG=
if [[ "$1" == "-C" ]]; then
  FS_CONFIG=$2
  shift; shift
fi

BLOCK_LIST=
if [[ "$1" == "-B" ]]; then
  BLOCK_LIST=$2
  shift; shift
fi

FC=$1

case $EXT_VARIANT in
  ext4) ;;
  *) echo "Only ext4 is supported!"; exit 3 ;;
esac

if [ -z $MOUNT_POINT ]; then
  echo "Mount point is required"
  exit 2
fi

if [ -z $SIZE ]; then
  echo "Need size of filesystem"
  exit 2
fi

OPT=""
if [ -n "$FC" ]; then
  OPT="$OPT -S $FC"
fi
if [ -n "$FS_CONFIG" ]; then
  OPT="$OPT -C $FS_CONFIG"
fi
if [ -n "$BLOCK_LIST" ]; then
  OPT="$OPT -B $BLOCK_LIST"
fi

MAKE_EXT4FS_CMD="make_ext4fs $ENABLE_SPARSE_IMAGE -T $TIMESTAMP $OPT -l $SIZE $JOURNAL_FLAGS -a $MOUNT_POINT $OUTPUT_FILE $SRC_DIR"
echo $MAKE_EXT4FS_CMD
$MAKE_EXT4FS_CMD
if [ $? -ne 0 ]; then
  exit 4
fi
