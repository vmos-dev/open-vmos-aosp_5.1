# Copyright (C) 2012 The Android Open Source Project
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


include $(CLEAR_VARS)

MOBICORE_PATH := hardware/samsung_slsi/exynos5/mobicore

LOCAL_MODULE := keystore.exynos5
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_SHARED_LIBRARIES)/hw
LOCAL_SRC_FILES := keymaster_mobicore.cpp tlcTeeKeymaster_if.c
LOCAL_C_INCLUDES := \
	external/openssl/include \
	$(MOBICORE_PATH)/daemon/ClientLib/public \
	$(MOBICORE_PATH)/common/MobiCore/inc/
LOCAL_C_FLAGS = -fvisibility=hidden -Wall -Werror
LOCAL_SHARED_LIBRARIES := libcrypto liblog libMcClient
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

include $(BUILD_SHARED_LIBRARY)
