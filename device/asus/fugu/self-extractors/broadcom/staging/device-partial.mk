# Copyright 2014 The Android Open Source Project
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

# Broadcom blob(s) necessary for Fugu hardware
PRODUCT_COPY_FILES := \
    vendor/broadcom/fugu/proprietary/BtFwLoader:system/bin/BtFwLoader:broadcom \
    vendor/broadcom/fugu/proprietary/bcmdhd.cal:system/etc/wifi/bcmdhd.cal:broadcom \
    vendor/broadcom/fugu/proprietary/bcmdhd_sr2.cal:system/etc/wifi/bcmdhd_sr2.cal:broadcom \
    vendor/broadcom/fugu/proprietary/BCM4350C0.hcd:system/vendor/firmware/BCM4350C0.hcd:broadcom \
    vendor/broadcom/fugu/proprietary/BCM4350C0_SR2.hcd:system/vendor/firmware/BCM4350C0_SR2.hcd:broadcom \
    vendor/broadcom/fugu/proprietary/wlutil:system/xbin/wlutil:broadcom \

