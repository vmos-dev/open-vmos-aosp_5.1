#
# Copyright (C) 2014 The Android Open-Source Project
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

$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/aosp_base_telephony.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/board/generic_arm64/device.mk)

PRODUCT_RUNTIMES := runtime_libart_default

PRODUCT_NAME := ranchu_arm64
# Use the same BoardConfig as generic_arm64.
PRODUCT_DEVICE := generic_arm64
PRODUCT_BRAND := Android
PRODUCT_MODEL := AOSP on qemu arm64 emulator

PRODUCT_PACKAGES += \
    libGLES_android


TARGET_PROVIDES_INIT_RC := true
PRODUCT_COPY_FILES += $(call add-to-product-copy-files-if-exists,\
			system/core/rootdir/init.rc:root/init.rc \
			$(LOCAL_PATH)/init.ranchu.rc:root/init.ranchu.rc \
			$(LOCAL_PATH)/fstab.ranchu:root/fstab.ranchu \
			$(LOCAL_PATH)/ueventd.ranchu.rc:root/ueventd.ranchu.rc)
