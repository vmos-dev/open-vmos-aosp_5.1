#!/bin/sh

# Copyright 2010 The Android Open Source Project
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

# start jb-dev
# 368864 = JRN61B
# 371028 = JRN65
# 382301 = JRN75
# 386704 = JRN80
# 391496 = JRN83D
# 397816 = JRO03B
# 398337 = JRO03C
# 405518 = JRO03H
# 438695 = JRO03R
# 463694 = JZO54G
# 485486 = JZO54K
# end jb-dev
# start jb-mr1-dev
# 521994 = JOP32B
# 524024 = JOP36
# 527221 = JOP40
# 527662 = JOP40C
# 561924 = JOP40G
# end jb-mr1-dev
# start jb-mr1.1-dev
# 551245 = JDP82
# 573038 = JDQ39
# end jb-mr1.1-dev
# start jb-mr2-dev
# 638589 = JWR11D
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
if test $BRANCH = jb-dev
then
  ZIP=nakasi-ota-485486.zip
  BUILD=jzo54k
fi # jb-dev
if test $BRANCH = jb-mr1-dev
then
  ZIP=nakasi-ota-561924.zip
  BUILD=jop40g
fi # jb-mr1-dev
if test $BRANCH = jb-mr1.1-dev
then
  ZIP=nakasi-ota-573038.zip
  BUILD=jdq39
fi # jb-mr1.1-dev
if test $BRANCH = jb-mr2-dev
then
  ZIP=nakasi-ota-737497
  BUILD=jwr66v
fi # jb-mr2-dev
if test $BRANCH = klp-dev
then
  ZIP=nakasi-ota-937116
  BUILD=kot49h
fi # klp-dev
ROOTDEVICE=grouper
DEVICE=grouper
MANUFACTURER=asus

for COMPANY in asus broadcom elan invensense nvidia nxp widevine
do
  echo Processing files from $COMPANY
  rm -rf tmp
  FILEDIR=tmp/vendor/$COMPANY/$DEVICE/proprietary
  mkdir -p $FILEDIR
  mkdir -p tmp/vendor/$MANUFACTURER/$ROOTDEVICE
  case $COMPANY in
  asus)
    TO_EXTRACT="\
            system/bin/sensors-config \
            system/lib/hw/camera.tegra3.so \
            system/lib/hw/sensors.grouper.so \
            system/lib/libsensors.lightsensor.so \
            system/vendor/lib/drm/libdrmwvmplugin.so \
            system/vendor/lib/libwvm.so \
            "
    ;;
  broadcom)
    TO_EXTRACT="\
            system/bin/glgps \
            system/etc/firmware/bcm4330.hcd \
            system/etc/gps/gpsconfig.xml \
            system/lib/hw/gps.tegra3.so \
            "
    ;;
  elan)
    TO_EXTRACT="\
            system/etc/firmware/touch_fw.ekt \
            "
    ;;
  invensense)
    TO_EXTRACT="\
            system/lib/libinvensense_hal.so \
            system/lib/libmllite.so \
            system/lib/libmplmpu.so \
            "
    ;;
  nvidia)
    TO_EXTRACT="\
            system/bin/tf_daemon \
            system/etc/firmware/nvavp_os_00001000.bin \
            system/etc/firmware/nvavp_os_0ff00000.bin \
            system/etc/firmware/nvavp_os_e0000000.bin \
            system/etc/firmware/nvavp_os_eff00000.bin \
            system/etc/firmware/nvavp_vid_ucode_alt.bin \
            system/etc/nvcamera.conf \
            system/etc/nvram.txt \
            system/lib/egl/libEGL_tegra.so \
            system/lib/egl/libGLESv1_CM_tegra.so \
            system/lib/egl/libGLESv2_tegra.so \
            system/lib/hw/gralloc.tegra3.so \
            system/lib/hw/hwcomposer.tegra3.so \
            system/lib/libardrv_dynamic.so \
            system/lib/libcgdrv.so \
            system/lib/libnvapputil.so \
            system/lib/libnvasfparserhal.so \
            system/lib/libnvaviparserhal.so \
            system/lib/libnvavp.so \
            system/lib/libnvcamerahdr.so \
            system/lib/libnvddk_2d.so \
            system/lib/libnvddk_2d_v2.so \
            system/lib/libnvdispmgr_d.so \
            system/lib/libnvmm.so \
            system/lib/libnvmmlite.so \
            system/lib/libnvmmlite_audio.so \
            system/lib/libnvmmlite_image.so \
            system/lib/libnvmmlite_utils.so \
            system/lib/libnvmmlite_video.so \
            system/lib/libnvmm_audio.so \
            system/lib/libnvmm_camera.so \
            system/lib/libnvmm_contentpipe.so \
            system/lib/libnvmm_image.so \
            system/lib/libnvmm_manager.so \
            system/lib/libnvmm_misc.so \
            system/lib/libnvmm_parser.so \
            system/lib/libnvmm_service.so \
            system/lib/libnvmm_utils.so \
            system/lib/libnvmm_video.so \
            system/lib/libnvmm_writer.so \
            system/lib/libnvodm_dtvtuner.so \
            system/lib/libnvodm_hdmi.so \
            system/lib/libnvodm_imager.so \
            system/lib/libnvodm_misc.so \
            system/lib/libnvodm_query.so \
            system/lib/libnvomx.so \
            system/lib/libnvomxilclient.so \
            system/lib/libnvos.so \
            system/lib/libnvparser.so \
            system/lib/libnvrm.so \
            system/lib/libnvrm_graphics.so \
            system/lib/libnvsm.so \
            system/lib/libnvtvmr.so \
            system/lib/libnvwinsys.so \
            system/lib/libnvwsi.so \
            system/lib/libstagefrighthw.so \
            system/lib/libtf_crypto_sst.so \
            "
    ;;
  nxp)
    TO_EXTRACT="\
            system/vendor/firmware/libpn544_fw.so \
            "
    ;;
  widevine)
    TO_EXTRACT="\
            system/lib/libdrmdecrypt.so \
            system/vendor/lib/libwvdrm_L1.so \
            system/vendor/lib/libWVStreamControlAPI_L1.so
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
