# Copyright (C) 2008 The Android Open Source Project
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
# Modified 2011 by InvenSense, Inc


LOCAL_PATH := $(call my-dir)

ifneq ($(BOARD_USES_GENERIC_INVENSENSE),false)

# InvenSense fragment of the HAL
include $(CLEAR_VARS)

LOCAL_MODULE := libinvensense_hal

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := -DLOG_TAG=\"Sensors\"
LOCAL_CFLAGS += -DCONFIG_MPU_SENSORS_MPU3050=1

LOCAL_SRC_FILES := SensorBase.cpp MPLSensor.cpp 

LOCAL_C_INCLUDES += hardware/invensense/60xx/mlsdk/platform/include
LOCAL_C_INCLUDES += hardware/invensense/60xx/mlsdk/platform/include/linux
LOCAL_C_INCLUDES += hardware/invensense/60xx/mlsdk/platform/linux
LOCAL_C_INCLUDES += hardware/invensense/60xx/mlsdk/mllite
LOCAL_C_INCLUDES += hardware/invensense/60xx/mlsdk/mldmp
LOCAL_C_INCLUDES += hardware/invensense/60xx/mlsdk/external/aichi
LOCAL_C_INCLUDES += hardware/invensense/60xx/mlsdk/external/akmd

LOCAL_SHARED_LIBRARIES := liblog libcutils libutils libdl libmllite libmlplatform
LOCAL_CPPFLAGS+=-DLINUX=1
LOCAL_LDFLAGS:=-rdynamic
LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)

endif
