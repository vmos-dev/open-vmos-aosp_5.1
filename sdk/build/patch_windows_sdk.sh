#!/bin/bash
#
# Copyright (C) 2009 The Android Open Source Project
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


# This file is run by development/tools/build/windows_sdk.mk right
# after development.git/tools/build/patch_windows_sdk.sh.
# Please see the details in the other file.

set -e # any error stops the build

# Verbose by default. Use -q to make more silent.
if [[ "$1" == "-q" ]]; then
  shift
else
  echo "Win SDK: $0 $*"
  set -x # show bash commands
fi

TEMP_SDK_DIR=$1
WIN_OUT_DIR=$2
TOPDIR=${TOPDIR:-$3}

# Invoke atree to copy the files
if [[ -f ${TOPDIR}sdk/build/tools.windows.atree ]]; then
  ${TOPDIR}out/host/linux-x86/bin/atree -f ${TOPDIR}sdk/build/tools.windows.atree \
        -I $WIN_OUT_DIR/host/windows-x86 \
        -I ${TOPDIR:-.} \
        -o $TEMP_SDK_DIR
fi

