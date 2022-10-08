#
# Copyright 2013 The Android Open-Source Project
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

# rild
PRODUCT_PACKAGES := \
    rild \
    BasicSmsReceiver

PRODUCT_COPY_FILES := \
    device/asus/deb/fstab.deb:root/fstab.flo \
    device/asus/deb/init.deb.rc:root/init.flo.rc

PRODUCT_PACKAGES += \
	camera.deb

#NFC
PRODUCT_PACKAGES += \
    nfc_nci.bcm2079x.default

# Do not power down SIM card when modem is sent to Low Power Mode.
PRODUCT_PROPERTY_OVERRIDES += \
        persist.radio.apm_sim_not_pwdn=1

#Stop rild if non 3G SKU
PRODUCT_PACKAGES += \
        init.qcom.class_main.sh

PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
        rild.libpath=/system/lib/libril-qc-qmi-1.so

# the actual meat of the device-specific product definition
$(call inherit-product, device/asus/flo/device-common.mk)

# inherit from the non-open-source side, if present
$(call inherit-product-if-exists, vendor/asus/deb/device-vendor.mk)

DEVICE_PACKAGE_OVERLAYS := device/asus/deb/overlay
