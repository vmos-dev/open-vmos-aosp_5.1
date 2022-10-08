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

-include hardware/broadcom/wlan/bcmdhd/config/config-bcm.mk

BCM_FW_SRC_FILE_STA := fw_bcmdhd.bin
BCM_FW_SRC_FILE_AP  := fw_bcmdhd_apsta.bin

PRODUCT_COPY_FILES += \
    hardware/broadcom/wlan/bcmdhd/firmware/bcm4339/$(BCM_FW_SRC_FILE_STA):$(TARGET_COPY_OUT_VENDOR)/firmware/fw_bcmdhd.bin \
    hardware/broadcom/wlan/bcmdhd/firmware/bcm4339/$(BCM_FW_SRC_FILE_AP):$(TARGET_COPY_OUT_VENDOR)/firmware/fw_bcmdhd_apsta.bin
