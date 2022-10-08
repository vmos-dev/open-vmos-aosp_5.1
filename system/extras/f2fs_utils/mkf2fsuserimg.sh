#!/bin/bash
#
# To call this script, make sure make_f2fs is somewhere in PATH

function usage() {
cat<<EOT
Usage:
${0##*/} OUTPUT_FILE SIZE
EOT
}

echo "in mkf2fsuserimg.sh PATH=$PATH"

if [ $# -lt 2 ]; then
  usage
  exit 1
fi

OUTPUT_FILE=$1
SIZE=$2
shift; shift


if [ -z $SIZE ]; then
  echo "Need size of filesystem"
  exit 2
fi

MAKE_F2FS_CMD="make_f2fs -l $SIZE $OUTPUT_FILE"
echo $MAKE_F2FS_CMD
$MAKE_F2FS_CMD
if [ $? -ne 0 ]; then
  exit 4
fi
