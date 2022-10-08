#!/bin/sh

# Copyright 2012 The Android Open Source Project
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

# start jb-mr1-dev
# 454897 = JOO61E
# 474128 = JOO86
# 476441 = JOO87B
# 483070 = JOP01
# 521994 = JOP32B
# 524024 = JOP36
# 526897 = JOP39B
# 527662 = JOP40C
# end jb-mr1-dev
# start jb-mr1.1-dev
# 551245 = JDP82
# 573038 = JDQ39
# end jb-mr1.1-dev
# start jb-mr2-dev
# 637162 = JWR11B
# 681336 = JWR50
# 683083 = JWR51
# 684634 = JWR52
# 686185 = JWR53
# 689345 = JWR58
# 690834 = JWR59
# 692263 = JWR60
# 695489 = JWR64
# 699533 = JWR66
# 701448 = JWR66C
# 704243 = JWR66G
# 711294 = JWR66N
# 736095 = JWR66U
# 737497 = JWR66V
# end jb-mr2-dev
BRANCH=klp-dev
if test $BRANCH = jb-mr1-dev
then
  ZIP=mantaray-ota-527662
  BUILD=jop40c
fi # jb-mr1-dev
if test $BRANCH = jb-mr1.1-dev
then
  ZIP=mantaray-ota-573038
  BUILD=jdq39
fi # jb-mr1.1-dev
if test $BRANCH = jb-mr2-dev
then
  ZIP=mantaray-ota-737497
  BUILD=jwr66v
fi # jb-mr2-dev
if test $BRANCH = klp-dev
then
  ZIP=mantaray-ota-937116
  BUILD=kot49h
fi # klp-dev
ROOTDEVICE=manta
DEVICE=manta
MANUFACTURER=samsung

for COMPANY in audience broadcom samsung
do
  echo Processing files from $COMPANY
  rm -rf tmp
  FILEDIR=tmp/vendor/$COMPANY/$DEVICE/proprietary
  mkdir -p $FILEDIR
  mkdir -p tmp/vendor/$MANUFACTURER/$ROOTDEVICE
  case $COMPANY in
  audience)
    TO_EXTRACT="\
            system/vendor/firmware/es305_fw.bin \
            "
    ;;
  broadcom)
    TO_EXTRACT="\
            system/vendor/firmware/bcm2079x_firmware.ncd \
            system/vendor/firmware/bcm2079x_pre_firmware.ncd \
            system/vendor/firmware/bcm43241.hcd \
            "
    ;;
  samsung)
    TO_EXTRACT="\
            system/vendor/firmware/fimc_is_fw.bin \
            system/vendor/firmware/fimc_is_fw2.bin \
            system/vendor/firmware/maxtouch.fw \
            system/vendor/firmware/mfc_fw.bin \
            system/vendor/firmware/setfile.bin \
            system/vendor/firmware/setfile_4e5.bin \
            system/vendor/firmware/setfile_6a3.bin \
            system/vendor/lib/egl/libGLES_mali.so \
            system/vendor/lib/libdrmdecrypt.so \
            system/vendor/lib/libmalicore.bc \
            system/vendor/lib/libRSDriverArm.so \
            system/vendor/lib/libbccArm.so \
            system/vendor/lib/libstagefright_hdcp.so \
            system/vendor/secapp/00060308060501020000000000000000.tlbin \
            system/vendor/secapp/020a0000000000000000000000000000.drbin \
            system/vendor/secapp/07060000000000000000000000000000.tlbin \
            system/vendor/secapp/ffffffff000000000000000000000005.tlbin \
            "
    ;;
  esac
  echo \ \ Extracting files from OTA package
  for ONE_FILE in $TO_EXTRACT
  do
    echo \ \ \ \ Extracting $ONE_FILE
    unzip -j -o $ZIP $ONE_FILE -d $FILEDIR > /dev/null || echo \ \ \ \ Error extracting $ONE_FILE
    if test $ONE_FILE = system/vendor/bin/gpsd -o $ONE_FILE = system/vendor/bin/pvrsrvinit -o $ONE_FILE = system/bin/fRom
    then
      chmod a+x $FILEDIR/$(basename $ONE_FILE) || echo \ \ \ \ Error chmoding $ONE_FILE
    fi
  done
  echo \ \ Setting up $COMPANY-specific makefiles
  cp -R $COMPANY/staging/* tmp/vendor/$COMPANY/$DEVICE || echo \ \ \ \ Error copying makefiles
  echo \ \ Setting up shared makefiles
  cp -R root/* tmp/vendor/$MANUFACTURER/$ROOTDEVICE || echo \ \ \ \ Error copying makefiles
  echo \ \ Generating self-extracting script
  SCRIPT=extract-$COMPANY-$DEVICE.sh
  cat PROLOGUE > tmp/$SCRIPT || echo \ \ \ \ Error generating script
  cat $COMPANY/COPYRIGHT >> tmp/$SCRIPT || echo \ \ \ \ Error generating script
  cat PART1 >> tmp/$SCRIPT || echo \ \ \ \ Error generating script
  cat $COMPANY/LICENSE >> tmp/$SCRIPT || echo \ \ \ \ Error generating script
  cat PART2 >> tmp/$SCRIPT || echo \ \ \ \ Error generating script
  echo tail -n +$(expr 2 + $(cat PROLOGUE $COMPANY/COPYRIGHT PART1 $COMPANY/LICENSE PART2 PART3 | wc -l)) \$0 \| tar zxv >> tmp/$SCRIPT || echo \ \ \ \ Error generating script
  cat PART3 >> tmp/$SCRIPT || echo \ \ \ \ Error generating script
  (cd tmp ; tar zc --owner=root --group=root vendor/ >> $SCRIPT || echo \ \ \ \ Error generating embedded tgz)
  chmod a+x tmp/$SCRIPT || echo \ \ \ \ Error generating script
  ARCHIVE=$COMPANY-$DEVICE-$BUILD-$(md5sum < tmp/$SCRIPT | cut -b -8 | tr -d \\n).tgz
  rm -f $ARCHIVE
  echo \ \ Generating final archive
  (cd tmp ; tar --owner=root --group=root -z -c -f ../$ARCHIVE $SCRIPT || echo \ \ \ \ Error archiving script)
  rm -rf tmp
done
