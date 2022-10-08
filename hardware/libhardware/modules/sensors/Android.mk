#
# Copyright (C) 2013 The Android Open-Source Project
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

LOCAL_PATH := $(call my-dir)

ifeq ($(USE_SENSOR_MULTI_HAL),true)

include $(CLEAR_VARS)

LOCAL_MODULE := sensors.$(TARGET_DEVICE)

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

LOCAL_CFLAGS := -DLOG_TAG=\"MultiHal\"

LOCAL_SRC_FILES := \
    multihal.cpp \
    SensorEventQueue.h \
    SensorEventQueue.cpp \

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    liblog \
    libstlport \
    libutils \

LOCAL_PRELINK_MODULE := false
LOCAL_STRIP_MODULE := false

LOCAL_C_INCLUDES := \
    external/stlport/stlport \
    bionic \

include $(BUILD_SHARED_LIBRARY)

endif # USE_SENSOR_MULTI_HAL

include $(call all-makefiles-under, $(LOCAL_PATH))
