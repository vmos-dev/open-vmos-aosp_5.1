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

rm -f extract-lists.txt
cat ../vendor_owner_info.txt |
cut -d : -f 2 |
sort -u |
grep -v google |
while read target_owner
do
cat > $target_owner/staging/device-partial.mk << EOF
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

EOF
echo -n "# " >> $target_owner/staging/device-partial.mk
case $target_owner in
asus)
echo -n Asus >> $target_owner/staging/device-partial.mk
;;
broadcom)
echo -n Broadcom >> $target_owner/staging/device-partial.mk
;;
qcom)
echo -n Qualcomm >> $target_owner/staging/device-partial.mk
;;
esac
echo " blob(s) necessary for Deb hardware" >> $target_owner/staging/device-partial.mk
echo "PRODUCT_COPY_FILES := \\" >> $target_owner/staging/device-partial.mk

echo "  $target_owner)" >> extract-lists.txt
echo "    TO_EXTRACT=\"\\" >> extract-lists.txt


cat ../proprietary-blobs.txt |
grep ^/ |
cut -b 2- |
sort |
while read file
do

auto_owner=$(grep ^$file: ../vendor_owner_info.txt | cut -d : -f 2)
if test $file = system/bin/mm-qcamera-app -o $file = system/lib/hw/camera.deb.so -o $file = system/lib/hw/gps.msm8960.so -o $file = system/lib/libgps.utils.so -o $file = system/lib/libloc_adapter.so -o $file = system/lib/libloc_eng.so -o $file = system/lib/libmmcamera_interface.so -o $file = system/lib/libmmjpeg_interface.so -o $file = system/lib/libqomx_core.so
then
auto_owner=qcom
fi

if test "$auto_owner" = "" -a $file != system/etc/firmware/tzapps.b00 -a $file != system/etc/firmware/tzapps.b01 -a $file != system/etc/firmware/tzapps.b02 -a $file != system/etc/firmware/tzapps.b03 -a $file != system/etc/firmware/tzapps.mdt -a $file != system/etc/firmware/wcd9310/wcd9310_anc.bin -a $file != system/etc/firmware/wcd9310/wcd9310_mbhc.bin
then
echo $file has no known owner
fi

if test "$auto_owner" = "$target_owner"
then
if test $file != system/lib/libacdbloader.so -a $file != system/vendor/firmware/tzapps.mdt
then
echo "    vendor/$target_owner/deb/proprietary/$(basename $file):$file:$target_owner \\" >> $target_owner/staging/device-partial.mk
fi
echo "            $file \\" >> extract-lists.txt
fi
done

echo >> $target_owner/staging/device-partial.mk
if test $target_owner = qcom
then
echo PRODUCT_PACKAGES := libacdbloader tzapps.mdt >> $target_owner/staging/device-partial.mk
fi

echo "            \"" >> extract-lists.txt
echo "    ;;" >> extract-lists.txt
done
