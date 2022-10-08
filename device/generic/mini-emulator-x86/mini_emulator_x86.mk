# Copyright (C) 2013 The Android Open Source Project
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

$(call inherit-product, device/generic/x86/mini_x86.mk)

$(call inherit-product, device/generic/mini-emulator-armv7-a-neon/mini_emulator_common.mk)

PRODUCT_NAME := mini_emulator_x86
PRODUCT_DEVICE := mini-emulator-x86
PRODUCT_BRAND := Android
PRODUCT_MODEL := mini-emulator-x86

LOCAL_KERNEL := prebuilts/qemu-kernel/x86/kernel-qemu
PRODUCT_COPY_FILES += \
    $(LOCAL_KERNEL):kernel
