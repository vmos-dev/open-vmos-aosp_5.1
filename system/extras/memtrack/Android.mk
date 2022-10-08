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

LOCAL_PATH:= $(call my-dir)

src_files := \
	memtrack.cpp

includes := \
    bionic \
    external/stlport/stlport \

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(src_files)

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE := memtrack_share

LOCAL_C_INCLUDES += $(includes)
LOCAL_SHARED_LIBRARIES := \
	libc \
	libstlport \
	liblog \

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(src_files)
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE := memtrack

LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_C_INCLUDES += $(includes)
LOCAL_STATIC_LIBRARIES := \
	libc \
	libstdc++ \
	libstlport_static \
	liblog \

include $(BUILD_EXECUTABLE)
