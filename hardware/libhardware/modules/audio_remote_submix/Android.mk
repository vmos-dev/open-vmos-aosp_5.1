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

LOCAL_MODULE := audio.r_submix.default
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := \
	audio_hw.cpp
LOCAL_C_INCLUDES += \
	frameworks/av/include/ \
	frameworks/native/include/
LOCAL_SHARED_LIBRARIES := liblog libcutils libutils libnbaio
LOCAL_STATIC_LIBRARIES := libmedia_helper
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -Wno-unused-parameter

include $(BUILD_SHARED_LIBRARY)

