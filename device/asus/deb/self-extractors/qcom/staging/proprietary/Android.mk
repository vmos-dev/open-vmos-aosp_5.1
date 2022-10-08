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

LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_DEVICE),deb)

include $(CLEAR_VARS)
LOCAL_MODULE := libacdbloader
LOCAL_SRC_FILES := libacdbloader.so
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := qcom

# Create symbolic link because user space can access persist directory,
# while kernel ALSA drivers can only access the /system/etc/firmware directory
LOCAL_POST_INSTALL_CMD := \
    mkdir -p $(TARGET_OUT_ETC)/firmware/wcd9310; \
        ln -sf /data/misc/audio/wcd9310_anc.bin \
        $(TARGET_OUT_ETC)/firmware/wcd9310/wcd9310_anc.bin; \
        ln -sf /data/misc/audio/mbhc.bin \
        $(TARGET_OUT_ETC)/firmware/wcd9310/wcd9310_mbhc.bin

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := tzapps.mdt
LOCAL_MODULE_OWNER := qcom
LOCAL_SRC_FILES := tzapps.mdt
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/firmware
LOCAL_POST_INSTALL_CMD := \
        mkdir -p $(TARGET_OUT)/etc/firmware/; \
        ln -sf /system/vendor/firmware/tzapps.b00 $(TARGET_OUT)/etc/firmware/tzapps.b00; \
        ln -sf /system/vendor/firmware/tzapps.b01 $(TARGET_OUT)/etc/firmware/tzapps.b01; \
        ln -sf /system/vendor/firmware/tzapps.b02 $(TARGET_OUT)/etc/firmware/tzapps.b02; \
        ln -sf /system/vendor/firmware/tzapps.b03 $(TARGET_OUT)/etc/firmware/tzapps.b03; \
        ln -sf /system/vendor/firmware/tzapps.mdt $(TARGET_OUT)/etc/firmware/tzapps.mdt;
include $(BUILD_PREBUILT)

endif
