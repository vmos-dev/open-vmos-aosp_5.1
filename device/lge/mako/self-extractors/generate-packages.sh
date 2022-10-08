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

# start jb-mr1.1-dev
# 569806 = JDQ36B
# 573038 = JDQ39
# end jb-mr1.1-dev
# start jb-mr2-dev
# 686185 = JWR53
# 689345 = JWR58
# 690834 = JWR59
# 692263 = JWR60
# 695489 = JWR64
# 699533 = JWR66
# 701448 = JWR66C
# 704243 = JWR66G
# 709441 = JWR66M
# 711294 = JWR66N
# 736095 = JWR66U
# 737497 = JWR66V
# end jb-mr2-dev
BRANCH=klp-dev
if test $BRANCH = jb-mr1.1-dev
then
  ZIP=occam-ota-573038
  BUILD=jdq39
fi # jb-mr1.1-dev
if test $BRANCH = jb-mr2-dev
then
  ZIP=occam-ota-737497
  BUILD=jwr66v
fi # jb-mr2-dev
if test $BRANCH = klp-dev
then
  ZIP=occam-ota-937116
  BUILD=kot49h
fi # klp-dev
ROOTDEVICE=mako
DEVICE=mako
MANUFACTURER=lge

for COMPANY in broadcom lge qcom
do
  echo Processing files from $COMPANY
  rm -rf tmp
  FILEDIR=tmp/vendor/$COMPANY/$DEVICE/proprietary
  mkdir -p $FILEDIR
  mkdir -p tmp/vendor/$MANUFACTURER/$ROOTDEVICE
  case $COMPANY in
  broadcom)
    TO_EXTRACT="\
            system/vendor/firmware/bcm2079x_firmware.ncd \
            system/vendor/firmware/bcm2079x_pre_firmware.ncd \
            "
    ;;
  lge)
    TO_EXTRACT="\
            system/lib/libacdbdata.so \
            system/lib/libAKM.so \
            system/lib/libcamera_fast_af.so \
            system/lib/libmorpho_noise_reduction.so \
            system/vendor/firmware/keymaster/keymaster.b00 \
            system/vendor/firmware/keymaster/keymaster.b01 \
            system/vendor/firmware/keymaster/keymaster.b02 \
            system/vendor/firmware/keymaster/keymaster.b03 \
            system/vendor/firmware/keymaster/keymaster.mdt \
            "
    ;;
  qcom)
    TO_EXTRACT="\
            system/bin/ATFWD-daemon \
            system/bin/bridgemgrd \
            system/bin/btnvtool \
            system/bin/diag_klog \
            system/bin/diag_mdlog \
            system/bin/ds_fmc_appd \
            system/bin/efsks \
            system/bin/hci_qcomm_init \
            system/bin/ks \
            system/bin/mm-qcamera-daemon \
            system/bin/mpdecision \
            system/bin/netmgrd \
            system/bin/nl_listener \
            system/bin/port-bridge \
            system/bin/qcks \
            system/bin/qmuxd \
            system/bin/qseecomd \
            system/bin/radish \
            system/bin/rmt_storage \
            system/bin/sensors.qcom \
            system/bin/thermald \
            system/bin/usbhub \
            system/bin/usbhub_init \
            system/bin/v4l2-qcamera-app \
            system/etc/firmware/tzapps.b00 \
            system/etc/firmware/tzapps.b01 \
            system/etc/firmware/tzapps.b02 \
            system/etc/firmware/tzapps.b03 \
            system/etc/firmware/tzapps.mdt \
            system/etc/firmware/vidc_1080p.fw \
            system/etc/firmware/vidc.b00 \
            system/etc/firmware/vidc.b01 \
            system/etc/firmware/vidc.b02 \
            system/etc/firmware/vidc.b03 \
            system/etc/firmware/vidc.mdt \
            system/vendor/lib/egl/eglsubAndroid.so \
            system/vendor/lib/egl/libEGL_adreno.so \
            system/vendor/lib/egl/libGLESv1_CM_adreno.so \
            system/vendor/lib/egl/libGLESv2_adreno.so \
            system/vendor/lib/egl/libplayback_adreno.so \
            system/vendor/lib/egl/libq3dtools_adreno.so \
            system/lib/hw/flp.msm8960.so \
            system/lib/hw/gps.msm8960.so \
            system/lib/hw/sensors.msm8960.so \
            system/lib/libacdbloader.so \
            system/vendor/lib/libadreno_utils.so \
            system/lib/libaudcal.so \
            system/lib/libaudioalsa.so \
            system/vendor/lib/libC2D2.so \
            system/vendor/lib/libc2d30-a3xx.so \
            system/vendor/lib/libc2d30.so \
            system/vendor/lib/libCB.so \
            system/lib/libchromatix_imx111_default_video.so \
            system/lib/libchromatix_imx111_preview.so \
            system/lib/libchromatix_imx111_zsl.so \
            system/lib/libchromatix_imx119_default_video.so \
            system/lib/libchromatix_imx119_preview.so \
            system/lib/libchromatix_mt9e013_default_video.so \
            system/lib/libchromatix_mt9e013_preview.so \
            system/lib/libchromatix_mt9e013_video_hfr.so \
            system/lib/libchromatix_ov5647_default_video.so \
            system/lib/libchromatix_ov5647_preview.so \
            system/lib/libchromatix_ov5647_video_hfr.so \
            system/lib/libchromatix_ov8825_default_video.so \
            system/lib/libchromatix_ov8825_preview.so \
            system/lib/libchromatix_ov9726_default_video.so \
            system/lib/libchromatix_ov9726_preview.so \
            system/lib/libchromatix_s5k4e1_default_video.so \
            system/lib/libchromatix_s5k4e1_preview.so \
            system/lib/libchromatix_vx6953_default_video.so \
            system/lib/libchromatix_vx6953_preview.so \
            system/lib/libCommandSvc.so \
            system/lib/libconfigdb.so \
            system/lib/libcsd-client.so \
            system/lib/libdiag.so \
            system/lib/libdrmdiag.so \
            system/lib/libdrmfs.so \
            system/lib/libdrmtime.so \
            system/lib/libdsi_netctrl.so \
            system/lib/libdsprofile.so \
            system/lib/libdss.so \
            system/lib/libdsucsd.so \
            system/lib/libdsutils.so \
            system/lib/libgemini.so \
            system/lib/libgps.utils.so \
            system/vendor/lib/libgsl.so \
            system/lib/libI420colorconvert.so \
            system/lib/libidl.so \
            system/lib/libimage-jpeg-dec-omx-comp.so \
            system/lib/libimage-jpeg-enc-omx-comp.so \
            system/lib/libimage-omx-common.so \
            system/vendor/lib/libllvm-qcom.so \
            system/vendor/lib/libbccQTI.so \
            system/lib/libloc_core.so \
            system/lib/libloc_eng.so \
            system/lib/libmercury.so \
            system/lib/libmmcamera_faceproc.so \
            system/lib/libmmcamera_frameproc.so \
            system/lib/libmmcamera_hdr_lib.so \
            system/lib/libmmcamera_image_stab.so \
            system/lib/libmmcamera_statsproc31.so \
            system/lib/libmmcamera_wavelet_lib.so \
            system/lib/libmm-color-convertor.so \
            system/lib/libmmipl.so \
            system/lib/libmmjpeg.so \
            system/lib/libmmjps.so \
            system/lib/libmmmpod.so \
            system/lib/libmmmpo.so \
            system/lib/libmmstillomx.so \
            system/lib/libnetmgr.so \
            system/lib/liboemcamera.so \
            system/lib/liboemcrypto.so \
            system/lib/libqcci_legacy.so \
            system/lib/libqdi.so \
            system/lib/libqdp.so \
            system/lib/libqmi_cci.so \
            system/lib/libqmi_client_qmux.so \
            system/lib/libqmi_common_so.so \
            system/lib/libqmi_csi.so \
            system/lib/libqmi_csvt_srvc.so \
            system/lib/libqmi_encdec.so \
            system/lib/libqmiservices.so \
            system/lib/libqmi.so \
            system/lib/libQSEEComAPI.so \
            system/lib/libril-qc-qmi-1.so \
            system/lib/libril-qcril-hook-oem.so \
            system/vendor/lib/librs_adreno_sha1.so \
            system/vendor/lib/librs_adreno.so \
            system/vendor/lib/libRSDriver_adreno.so \
            system/vendor/lib/libsc-a3xx.so \
            system/lib/libsensor1.so \
            system/lib/libsensor_reg.so \
            system/lib/libsensor_user_cal.so \
            system/lib/libstagefright_hdcp.so \
            system/lib/libxml.so \
            system/vendor/firmware/a300_pfp.fw \
            system/vendor/firmware/a300_pm4.fw \
            system/vendor/firmware/dsps.b00 \
            system/vendor/firmware/dsps.b01 \
            system/vendor/firmware/dsps.b02 \
            system/vendor/firmware/dsps.b03 \
            system/vendor/firmware/dsps.b04 \
            system/vendor/firmware/dsps.b05 \
            system/vendor/firmware/dsps.mdt \
            system/vendor/firmware/q6.b00 \
            system/vendor/firmware/q6.b01 \
            system/vendor/firmware/q6.b03 \
            system/vendor/firmware/q6.b04 \
            system/vendor/firmware/q6.b05 \
            system/vendor/firmware/q6.b06 \
            system/vendor/firmware/q6.mdt \
            system/vendor/firmware/wcnss.b00 \
            system/vendor/firmware/wcnss.b01 \
            system/vendor/firmware/wcnss.b02 \
            system/vendor/firmware/wcnss.b04 \
            system/vendor/firmware/wcnss.b05 \
            system/vendor/firmware/wcnss.mdt \
            system/vendor/lib/libdrmdecrypt.so \
            system/vendor/lib/libgeofence.so \
            system/vendor/lib/libizat_core.so \
            system/vendor/lib/libloc_api_v02.so \
            system/vendor/lib/libloc_ds_api.so \
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
