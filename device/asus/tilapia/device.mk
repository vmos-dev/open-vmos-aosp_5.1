#
# Copyright (C) 2010 The Android Open Source Project
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
    device/asus/tilapia/fstab.grouper:root/fstab.grouper \
    device/asus/tilapia/init.recovery.tilapia.rc:root/init.recovery.grouper.rc \
    device/asus/tilapia/init.tilapia.rc:root/init.grouper.rc

DEVICE_PACKAGE_OVERLAYS := \
    device/asus/tilapia/overlay

# the actual meat of the device-specific product definition
$(call inherit-product, device/asus/grouper/device-common.mk)

# inherit from the non-open-source side, if present
$(call inherit-product-if-exists, vendor/asus/tilapia/device-vendor.mk)
