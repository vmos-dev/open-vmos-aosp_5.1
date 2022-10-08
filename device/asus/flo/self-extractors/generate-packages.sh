#!/bin/sh

# Copyright 2013 The Android Open Source Project
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

# start jb-mr2-dev
# 703372 = JSR71
# 704765 = JSR72
# 711747 = JSR78
# 713896 = JSR78B
# 719009 = JSR82
# 728843 = JSS01
# 730471 = JSS02B
# 739313 = JSS11D
# 741000 = JSS11I
# 741250 = JSS15
# 746990 = JSS15H
# 748502 = JSS15I
# 748593 = JSS15J
# 750418 = JSS15K
# end jb-mr2-dev
BRANCH=klp-dev
if test $BRANCH = klp-dev
then
  ZIP=razor-ota-937116
  BUILD=kot49h
fi # klp-dev
ROOTDEVICE=flo
DEVICE=flo
MANUFACTURER=asus

for COMPANY in asus broadcom qcom
do
  echo Processing files from $COMPANY
  rm -rf tmp
  FILEDIR=tmp/vendor/$COMPANY/$DEVICE/proprietary
  mkdir -p $FILEDIR
  mkdir -p tmp/vendor/$MANUFACTURER/$ROOTDEVICE
  case $COMPANY in
  asus)
    TO_EXTRACT="\
            system/lib/libacdbdata.so \
            system/lib/libAKM.so \
            "
    ;;
  broadcom)
    TO_EXTRACT="\
            system/vendor/firmware/bcm2079x-b5_firmware.ncd \
            system/vendor/firmware/bcm2079x-b5_pre_firmware.ncd \
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
            system/bin/irsc_util \
            system/bin/ks \
            system/bin/mm-qcamera-app \
            system/bin/mm-qcamera-daemon \
            system/bin/mm-qjpeg-enc-test \
            system/bin/mm-qomx-ienc-test \
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
            system/etc/DxHDCP.cfg \
            system/etc/firmware/vidc_1080p.fw \
            system/etc/firmware/vidc.b00 \
            system/etc/firmware/vidc.b01 \
            system/etc/firmware/vidc.b02 \
            system/etc/firmware/vidc.b03 \
            system/etc/firmware/vidcfw.elf \
            system/etc/firmware/vidc.mdt \
            system/etc/gps.conf \
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
            system/lib/libchromatix_ov5693_common.so \
            system/lib/libchromatix_ov5693_default_video.so \
            system/lib/libchromatix_ov5693_preview.so \
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
            system/lib/libgps.utils.so \
            system/vendor/lib/libgsl.so \
            system/lib/libI420colorconvert.so \
            system/lib/libidl.so \
            system/vendor/lib/libllvm-qcom.so \
            system/vendor/lib/libbccQTI.so \
            system/lib/libloc_core.so \
            system/lib/libloc_eng.so \
            system/lib/libmm-abl.so \
            system/lib/libmmcamera2_stats_algorithm.so \
            system/lib/libmmcamera_image_stab.so \
            system/lib/libmmcamera_mi1040.so \
            system/lib/libmmcamera_ov5693.so \
            system/lib/libmm-color-convertor.so \
            system/lib/libnetmgr.so \
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
            system/vendor/firmware/discretix/dxhdcp2.b00 \
            system/vendor/firmware/discretix/dxhdcp2.b01 \
            system/vendor/firmware/discretix/dxhdcp2.b02 \
            system/vendor/firmware/discretix/dxhdcp2.b03 \
            system/vendor/firmware/discretix/dxhdcp2.mdt \
            system/vendor/firmware/dsps.b00 \
            system/vendor/firmware/dsps.b01 \
            system/vendor/firmware/dsps.b02 \
            system/vendor/firmware/dsps.b03 \
            system/vendor/firmware/dsps.b04 \
            system/vendor/firmware/dsps.b05 \
            system/vendor/firmware/dsps.mdt \
            system/vendor/firmware/gss.b00 \
            system/vendor/firmware/gss.b01 \
            system/vendor/firmware/gss.b02 \
            system/vendor/firmware/gss.b03 \
            system/vendor/firmware/gss.b04 \
            system/vendor/firmware/gss.b05 \
            system/vendor/firmware/gss.b06 \
            system/vendor/firmware/gss.b07 \
            system/vendor/firmware/gss.b08 \
            system/vendor/firmware/gss.b09 \
            system/vendor/firmware/gss.b10 \
            system/vendor/firmware/gss.b11 \
            system/vendor/firmware/gss.mdt \
            system/vendor/firmware/keymaster/keymaster.b00 \
            system/vendor/firmware/keymaster/keymaster.b01 \
            system/vendor/firmware/keymaster/keymaster.b02 \
            system/vendor/firmware/keymaster/keymaster.b03 \
            system/vendor/firmware/keymaster/keymaster.mdt \
            system/vendor/firmware/q6.b00 \
            system/vendor/firmware/q6.b01 \
            system/vendor/firmware/q6.b03 \
            system/vendor/firmware/q6.b04 \
            system/vendor/firmware/q6.b05 \
            system/vendor/firmware/q6.b06 \
            system/vendor/firmware/q6.mdt \
            system/vendor/firmware/tzapps.b00 \
            system/vendor/firmware/tzapps.b01 \
            system/vendor/firmware/tzapps.b02 \
            system/vendor/firmware/tzapps.b03 \
            system/vendor/firmware/tzapps.mdt \
            system/vendor/firmware/wcnss.b00 \
            system/vendor/firmware/wcnss.b01 \
            system/vendor/firmware/wcnss.b02 \
            system/vendor/firmware/wcnss.b04 \
            system/vendor/firmware/wcnss.b05 \
            system/vendor/firmware/wcnss.mdt \
            system/vendor/lib/libdrmdecrypt.so \
            system/vendor/lib/libgemini.so \
            system/vendor/lib/libgeofence.so \
            system/vendor/lib/libimage-jpeg-enc-omx-comp.so \
            system/vendor/lib/libimage-omx-common.so \
            system/vendor/lib/libizat_core.so \
            system/vendor/lib/libloc_api_v02.so \
            system/vendor/lib/libloc_ds_api.so \
            system/vendor/lib/libmmcamera2_c2d_module.so \
            system/vendor/lib/libmmcamera2_cpp_module.so \
            system/vendor/lib/libmmcamera2_iface_modules.so \
            system/vendor/lib/libmmcamera2_imglib_modules.so \
            system/vendor/lib/libmmcamera2_isp_modules.so \
            system/vendor/lib/libmmcamera2_pproc_modules.so \
            system/vendor/lib/libmmcamera2_sensor_modules.so \
            system/vendor/lib/libmmcamera2_stats_modules.so \
            system/vendor/lib/libmmcamera2_vpe_module.so \
            system/vendor/lib/libmmcamera2_wnr_module.so \
            system/vendor/lib/libmmcamera_faceproc.so \
            system/vendor/lib/libmmcamera_imglib.so \
            system/vendor/lib/libmmipl.so \
            system/vendor/lib/libmmjpeg.so \
            system/vendor/lib/libmmqjpeg_codec.so \
            system/vendor/lib/libmmstillomx.so \
            system/vendor/lib/liboemcamera.so \
            system/vendor/lib/libqomx_jpegenc.so \
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
