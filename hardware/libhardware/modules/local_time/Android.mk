# Copyright (C) 2011 The Android Open Source Project
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

# The default local time HAL module.  The default module simply uses the
# system's clock_gettime(CLOCK_MONOTONIC) and does not support HW slewing.
# Devices which use the default implementation should take care to ensure that
# the oscillator backing the CLOCK_MONOTONIC implementation is phase locked to
# the audio and video output hardware.  This default implementation is loaded
# if no other device specific modules are present. The exact load order can be
# seen in libhardware/hardware.c
#
# The format of the name is local_time.<hardware>.so
include $(CLEAR_VARS)

LOCAL_MODULE := local_time.default
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := local_time_hw.c
LOCAL_SHARED_LIBRARIES := liblog libcutils
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
