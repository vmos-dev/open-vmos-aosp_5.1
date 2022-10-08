# Copyright (C) 2009 The Android Open Source Project
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

ifeq ($(PRODUCT_OPENSRC_USE_PREBUILT),yes)

include $(CLEAR_VARS)

LOCAL_MODULE := sensors.goldfish
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_TAGS := optional

LOCAL_MULTILIB := both
LOCAL_SRC_FILES_arm := prebuilt/arm/sensors.goldfish.so
LOCAL_SRC_FILES_arm64 := prebuilt/arm64/sensors.goldfish.so

LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

LOCAL_UNINSTALLABLE_MODULE := true

$(shell mkdir -p $(TARGET_OUT)/lib/$(LOCAL_MODULE_RELATIVE_PATH)/)
$(shell cp -rf $(LOCAL_PATH)/prebuilt/arm/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)_result $(TARGET_OUT)/lib/$(LOCAL_MODULE_RELATIVE_PATH)/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX))
ifeq ($(TARGET_IS_64_BIT),true)
$(shell mkdir -p $(TARGET_OUT)/lib64/$(LOCAL_MODULE_RELATIVE_PATH)/)
$(shell cp -rf $(LOCAL_PATH)/prebuilt/arm64/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)_result $(TARGET_OUT)/lib64/$(LOCAL_MODULE_RELATIVE_PATH)/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX))
endif

include $(BUILD_PREBUILT)

else

# HAL module implemenation stored in
# hw/<SENSORS_HARDWARE_MODULE_ID>.<ro.hardware>.so
include $(CLEAR_VARS)

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SHARED_LIBRARIES := liblog libcutils
LOCAL_SRC_FILES := sensors_qemu.c
ifeq ($(TARGET_PRODUCT),vbox_x86)
LOCAL_MODULE := sensors.vbox_x86
else
LOCAL_MODULE := sensors.goldfish
endif
include $(BUILD_SHARED_LIBRARY)

endif