#!/bin/bash

# Quick script used to setup Eclipse for the ADT plugin build.
#
# usage:
#   setup_eclipse.sh [-p] <dest_dir>
#   -p: run Eclipse in the background and print its PID in dest_dir/eclipse.pid
#
# Workflow:
# - downloads & unpack Eclipse if necessary
# - *runs* it once


#-----------------
#
# Note: right now this is invoked by sdk/eclipse/doBuild.sh
# and it *MUST* be invoked with the following destination directory:
#
# $ setup_eclipse.sh /buildbot/eclipse-android/3.4.0/
#
#-----------------


set -e # abort this script early if any command fails

function die() {
  echo $@
  exit 1
}

V="--no-verbose"
if [[ "$1" == "-v" ]]; then
  V=""
  shift
fi

if [[ "-p" == "$1" ]]; then
  GET_PID="-p"
  shift
fi


BASE_DIR="$1"

[[ -n "$1" ]] || die "Usage: $0 <dest-dir>"

# URL for Eclipse Linux RCP.
DOWNLOAD_URL="http://archive.eclipse.org/technology/epp/downloads/release/helios/SR2/eclipse-rcp-helios-SR2-linux-gtk-x86_64.tar.gz"

# URL for CDT
CDT_DOWNLOAD_URL="http://download.eclipse.org/tools/cdt/releases/helios/dist/cdt-master-7.0.2.zip"

BIN="$BASE_DIR/eclipse/eclipse"           # path to installed binary
TARGZ="$BASE_DIR/${DOWNLOAD_URL##*/}"     # base dir + filename of the download URL
CDTZIP="$BASE_DIR/${CDT_DOWNLOAD_URL##*/}"

if [[ ! -f "$BIN" ]]; then
  echo "Downloading and installing Eclipse in $BASE_DIR."
  mkdir -p "$BASE_DIR"

  wget --continue $V --output-document="$TARGZ" "$DOWNLOAD_URL"
  echo "Unpacking $TARGZ"
  (cd "$BASE_DIR" && tar xzf "$TARGZ")

  wget --continue $V --output-document="$CDTZIP" "$CDT_DOWNLOAD_URL"
  echo "Unpacking $CDTZIP"
  (cd "$BASE_DIR/eclipse" && unzip -o "$CDTZIP")

  echo
  echo "*** WARNING: To setup Eclipse correctly, it must be ran at least once manually"
  echo "***          Eclipse will now start."
  echo
  if [[ -n "$GET_PID" ]]; then
    # if started from the automatic eclipse build, run Eclipse in the background
    "$BIN" &
    ECLIPSE_PID=$!
    echo "*** Eclipse started in background with PID $ECLIPSE_PID"
    echo "$ECLIPSE_PID" > "$BASE_DIR"/eclipse.pid
    sleep 5  # give some time for Eclipse to start and setup its environment
  else
    # if started manually, run Eclipse in the foreground
    "$BIN"
  fi
fi

