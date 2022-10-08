#!/bin/bash
# Expected arguments:
# $1 = out_dir
# $2 = ide qualifier
# $3 = zip qualifier

if [[ "Linux" != $(uname) ]]; then
    echo "$0: ADT IDE build script runs only on Linux"
    exit 0
fi

PROG_DIR=$(dirname "$0")

DEST_DIR="$1"
IDE_QUALIFIER="$2"
ZIP_QUALIFIER="$3"

function die() {
  echo "$*" > /dev/stderr
  echo "Usage: $0 dest_dir ide_qualifier zip_qualifier" > /dev/stderr
  exit 1
}

if [[ -z "$DEST_DIR" ]]; then die "## Error: Missing dest_dir"; fi
if [[ -z "$IDE_QUALIFIER" ]]; then die "## Error: Missing ide qualifier"; fi
if [[ -z "$ZIP_QUALIFIER" ]]; then die "## Error: Missing zip qualifier"; fi

ADT_IDE_DEST_DIR="$DEST_DIR" \
ADT_IDE_QUALIFIER="$IDE_QUALIFIER" \
ADT_IDE_ZIP_QUALIFIER="$ZIP_QUALIFIER" \
    make PRODUCT-sdk-adt_eclipse_ide

