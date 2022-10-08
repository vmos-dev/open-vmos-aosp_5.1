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

# The following products should be used to generate a minimal system image
# to be run under upstream QEMU (with a few Android-related patches). Note
# that this is different from running them under the Android emulator.

PRODUCT_MAKEFILES := \
    $(LOCAL_DIR)/qemu_arm.mk \
    $(LOCAL_DIR)/qemu_x86.mk \
    $(LOCAL_DIR)/qemu_mips.mk \
    $(LOCAL_DIR)/qemu_x86_64.mk \
    $(LOCAL_DIR)/qemu_arm64.mk \
    $(LOCAL_DIR)/qemu_mips64.mk \
    $(LOCAL_DIR)/ranchu_arm64.mk \
COMMON_GLOBAL_CFLAGS += -DUSES_NAM
USE_FFMPEG := true


COMMON_GLOBAL_CFLAGS += -DUSE_RING0_BINDER=0
